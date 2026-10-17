// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
//

// Native token validation DLL for UltraVNC VeNCrypt token auth.
//
// Supports three credential types:
//   1. TOTP (RFC 6238) codes from authenticator apps such as Microsoft
//      Authenticator, Google Authenticator, etc.
//   2. Server-issued 64-character session tokens for reconnects.
//   3. SCRAM-SHA-256 verifiers for SASL auth (VeNCrypt subtypes 263/264).
//
// Exports:
//   validate_token(user, token)
//   tv_has_management_ui()
//   tv_show_management_ui(HWND parent)
//
// TOTP secrets are read from Windows Credential Manager first (target name
// "UltraVNC_TOTP_<user>"), and fall back to a DPAPI-encrypted INI file next to
// this DLL (<this DLL>.credentials.ini) for service accounts such as SYSTEM whose
// Credential Manager vault is different from the interactive admin's vault.

#include "TokenCore.h"
#include "TokenValidatorPlugin.h"
#include "TokenRes.h"
#include "../common/ScramSha256.h"

#include <windows.h>
#include <bcrypt.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <vector>
#include <string>
#include <mutex>
#include <algorithm>

#pragma comment(lib, "bcrypt.lib")

namespace {

constexpr size_t SHA1_DIGEST_LEN = 20;

// ---------------------------------------------------------------------------
// HMAC-SHA1 using Windows CNG (BCrypt)
// ---------------------------------------------------------------------------
bool hmacSha1(const BYTE* key, size_t keyLen, const BYTE* msg, size_t msgLen, BYTE out[SHA1_DIGEST_LEN])
{
    BCRYPT_ALG_HANDLE hAlg = NULL;
    BCRYPT_HASH_HANDLE hHash = NULL;
    NTSTATUS status;

    status = BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_SHA1_ALGORITHM, NULL, BCRYPT_ALG_HANDLE_HMAC_FLAG);
    if (!BCRYPT_SUCCESS(status))
        return false;

    status = BCryptCreateHash(hAlg, &hHash, NULL, 0, const_cast<PUCHAR>(key), static_cast<ULONG>(keyLen), 0);
    if (!BCRYPT_SUCCESS(status))
    {
        BCryptCloseAlgorithmProvider(hAlg, 0);
        return false;
    }

    status = BCryptHashData(hHash, const_cast<PUCHAR>(msg), static_cast<ULONG>(msgLen), 0);
    if (BCRYPT_SUCCESS(status))
        status = BCryptFinishHash(hHash, out, SHA1_DIGEST_LEN, 0);

    BCryptDestroyHash(hHash);
    BCryptCloseAlgorithmProvider(hAlg, 0);
    return BCRYPT_SUCCESS(status);
}

// ---------------------------------------------------------------------------
// RFC 6238 TOTP helpers
// ---------------------------------------------------------------------------
uint64_t currentUnixTime()
{
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);
    ULARGE_INTEGER u;
    u.LowPart = ft.dwLowDateTime;
    u.HighPart = ft.dwHighDateTime;
    return (u.QuadPart - 116444736000000000ULL) / 10000000ULL;
}

uint32_t hotp(const BYTE* secret, size_t secretLen, uint64_t counter)
{
    BYTE counterBE[8];
    for (int i = 7; i >= 0; --i)
    {
        counterBE[i] = static_cast<BYTE>(counter & 0xFF);
        counter >>= 8;
    }

    BYTE digest[SHA1_DIGEST_LEN];
    if (!hmacSha1(secret, secretLen, counterBE, sizeof(counterBE), digest))
        return 0xFFFFFFFF;

    int offset = digest[SHA1_DIGEST_LEN - 1] & 0x0F;
    uint32_t binary = ((static_cast<uint32_t>(digest[offset] & 0x7F) << 24) |
                       (static_cast<uint32_t>(digest[offset + 1]) << 16) |
                       (static_cast<uint32_t>(digest[offset + 2]) << 8) |
                       (static_cast<uint32_t>(digest[offset + 3])));
    return binary % 1000000;
}

constexpr const char* kTotpLastStepSection = "TotpLastStep";
std::mutex g_totpMutex;

bool validateTotp(const char* user, const char* code)
{
    size_t codeLen = strlen(code);
    if (codeLen < 6 || codeLen > 8)
        return false;
    for (size_t i = 0; i < codeLen; ++i)
        if (code[i] < '0' || code[i] > '9')
            return false;

    std::string secretBase32;
    if (!TokenCore::ReadTotpSecret(user, secretBase32) || secretBase32.empty())
        return false;

    std::vector<BYTE> secret;
    if (!TokenCore::Base32Decode(secretBase32, secret) || secret.empty())
        return false;

    uint64_t t = currentUnixTime() / 30;
    uint32_t expected = static_cast<uint32_t>(atoi(code));

    // Allow current, previous and next time step to tolerate clock skew.
    uint64_t matchedStep = 0;
    bool matched = false;
    for (int64_t delta = -1; delta <= 1; ++delta)
    {
        uint64_t step = t + delta;
        if (hotp(secret.data(), secret.size(), step) == expected)
        {
            matchedStep = step;
            matched = true;
            break;
        }
    }
    if (!matched)
        return false;

    std::lock_guard<std::mutex> lock(g_totpMutex);
    std::string lastText;
    if (TokenCore::ReadProtectedValue(kTotpLastStepSection, user, lastText))
    {
        uint64_t lastStep = _strtoui64(lastText.c_str(), nullptr, 10);
        if (matchedStep <= lastStep)
            return false;
    }
    return TokenCore::WriteProtectedValue(kTotpLastStepSection, user, std::to_string(matchedStep));
}

bool isAllDigits(const char* s)
{
    if (!s || !*s)
        return false;
    for (const char* p = s; *p; ++p)
        if (*p < '0' || *p > '9')
            return false;
    return true;
}

// ---------------------------------------------------------------------------
// Management dialog
// ---------------------------------------------------------------------------
HINSTANCE g_hInst = NULL;
HBITMAP g_hQrBitmap = NULL;

void SetQrImage(HWND hDlg, const std::string& uri)
{
    HWND hStatic = GetDlgItem(hDlg, IDC_TV_QR_IMAGE);
    if (!hStatic)
        return;

    HBITMAP hBmp = TokenCore::RenderQrBitmap(hDlg, uri, 5, 4);
    if (!hBmp)
        return;

    HBITMAP hOld = (HBITMAP)SendMessageA(hStatic, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hBmp);
    if (hOld && hOld != hBmp)
        DeleteObject(hOld);

    if (g_hQrBitmap && g_hQrBitmap != hOld && g_hQrBitmap != hBmp)
        DeleteObject(g_hQrBitmap);

    g_hQrBitmap = hBmp;
}

void UpdateUriAndQr(HWND hDlg, const std::string& user, const std::string& secret)
{
    std::string uri = TokenCore::BuildOtpAuthUri(user, secret);
    SetDlgItemTextA(hDlg, IDC_TV_URI, uri.c_str());
    SetQrImage(hDlg, uri);
}

void RefreshUserList(HWND hDlg)
{
    HWND hList = GetDlgItem(hDlg, IDC_TV_USER_LIST);
    if (!hList)
        return;

    int prevSel = (int)SendMessageA(hList, LB_GETCURSEL, 0, 0);
    char prevUser[256] = {};
    if (prevSel != LB_ERR)
        SendMessageA(hList, LB_GETTEXT, prevSel, (LPARAM)prevUser);

    SendMessageA(hList, LB_RESETCONTENT, 0, 0);

    // Union of TOTP (CredMan + INI) and SCRAM (INI) users, with a factor badge.
    std::vector<std::string> totpUsers = TokenCore::ListTotpUsers();
    std::vector<std::string> scramUsers = TokenCore::ListProtectedKeys("SCRAM");

    std::vector<std::string> all = totpUsers;
    for (const auto& s : scramUsers)
        if (std::find(all.begin(), all.end(), s) == all.end())
            all.push_back(s);

    for (const auto& u : all)
    {
        bool hasTotp = std::find(totpUsers.begin(), totpUsers.end(), u) != totpUsers.end();
        bool hasScram = std::find(scramUsers.begin(), scramUsers.end(), u) != scramUsers.end();
        std::string display = u;
        if (hasTotp && hasScram)      display += " [TOTP+SCRAM]";
        else if (hasTotp)             display += " [TOTP]";
        else if (hasScram)            display += " [SCRAM]";

        int idx = (int)SendMessageA(hList, LB_ADDSTRING, 0, (LPARAM)display.c_str());
        if (idx != LB_ERR && strcmp(u.c_str(), prevUser) == 0)
            SendMessageA(hList, LB_SETCURSEL, idx, 0);
    }
}

// Strip display decorations (" [TOTP+SCRAM]", " (static fallback)") from a
// listbox string, leaving the bare username.
void StripListName(char* name)
{
    char* p = strstr(name, " [");
    char* q = strstr(name, " (");
    char* cut = (p && q) ? (p < q ? p : q) : (p ? p : q);
    if (cut)
        *cut = '\0';
}

void UpdateFactorsLabel(HWND hDlg, const char* user)
{
    if (!user || !*user)
    {
        SetDlgItemTextA(hDlg, IDC_TV_FACTORS, "");
        return;
    }
    bool hasTotp = false, hasScram = false;
    std::string tmp;
    std::vector<std::string> t = TokenCore::ListTotpUsers();
    hasTotp = std::find(t.begin(), t.end(), user) != t.end();
    hasScram = TokenCore::ReadProtectedValue("SCRAM", user, tmp) && !tmp.empty();

    std::string label = "Factors: ";
    if (hasTotp) label += "TOTP ";
    if (hasScram) label += "SCRAM";
    if (!hasTotp && !hasScram) label += "none";
    SetDlgItemTextA(hDlg, IDC_TV_FACTORS, label.c_str());
}

void OnUserSelectionChanged(HWND hDlg)
{
    HWND hList = GetDlgItem(hDlg, IDC_TV_USER_LIST);
    if (!hList)
        return;

    int idx = (int)SendMessageA(hList, LB_GETCURSEL, 0, 0);
    if (idx == LB_ERR)
    {
        SetDlgItemTextA(hDlg, IDC_TV_URI, "");
        UpdateFactorsLabel(hDlg, "");
        return;
    }

    char user[256] = {};
    SendMessageA(hList, LB_GETTEXT, idx, (LPARAM)user);
    StripListName(user);

    UpdateFactorsLabel(hDlg, user);

    std::string secret;
    if (TokenCore::ReadTotpSecret(user, secret) && !secret.empty())
    {
        UpdateUriAndQr(hDlg, user, secret);
    }
    else
    {
        SetDlgItemTextA(hDlg, IDC_TV_URI, "");
    }
}

bool GetSelectedUser(HWND hDlg, char* user, size_t userSize)
{
    HWND hList = GetDlgItem(hDlg, IDC_TV_USER_LIST);
    if (!hList)
        return false;

    int idx = (int)SendMessageA(hList, LB_GETCURSEL, 0, 0);
    if (idx == LB_ERR)
        return false;

    SendMessageA(hList, LB_GETTEXT, idx, (LPARAM)user);
    if (strstr(user, " (static fallback)"))
        return false; // read-only static fallback
    StripListName(user);

    return user[0] != '\0';
}

void OnAddUser(HWND hDlg)
{
    char user[256] = {};
    GetDlgItemTextA(hDlg, IDC_TV_USERNAME, user, sizeof(user));

    // Trim leading/trailing spaces.
    char* start = user;
    while (*start == ' ' || *start == '\t') ++start;
    char* end = user + strlen(user);
    while (end > start && (*(end - 1) == ' ' || *(end - 1) == '\t')) --end;
    *end = '\0';
    if (start != user)
        memmove(user, start, end - start + 1);

    if (strlen(user) == 0)
    {
        MessageBoxA(hDlg, "Enter a username.", "Token Manager", MB_OK | MB_ICONWARNING);
        return;
    }

    std::string secret;
    if (!TokenCore::GenerateRandomBase32Secret(secret))
    {
        MessageBoxA(hDlg, "Failed to generate a new secret.", "Token Manager", MB_OK | MB_ICONERROR);
        return;
    }

    if (!TokenCore::WriteTotpSecret(user, secret))
    {
        MessageBoxA(hDlg, "Failed to save the secret.\nMake sure the account running the dialog can write to Credential Manager and the INI fallback.",
                    "Token Manager", MB_OK | MB_ICONERROR);
        return;
    }

    RefreshUserList(hDlg);

    // Select the new user.
    HWND hList = GetDlgItem(hDlg, IDC_TV_USER_LIST);
    int count = (int)SendMessageA(hList, LB_GETCOUNT, 0, 0);
    for (int i = 0; i < count; ++i)
    {
        char item[256] = {};
        SendMessageA(hList, LB_GETTEXT, i, (LPARAM)item);
        if (strcmp(item, user) == 0)
        {
            SendMessageA(hList, LB_SETCURSEL, i, 0);
            break;
        }
    }

    UpdateUriAndQr(hDlg, user, secret);
    SetDlgItemTextA(hDlg, IDC_TV_USERNAME, "");

    // Also write a BMP file next to the INI for the administrator's convenience.
    char tempPath[MAX_PATH] = {};
    if (GetTempPathA(MAX_PATH, tempPath))
    {
        char bmpPath[MAX_PATH] = {};
        sprintf_s(bmpPath, "%s%s_uvnc_totp.bmp", tempPath, user);
        TokenCore::WriteQrBmpToFile(TokenCore::BuildOtpAuthUri(user, secret), bmpPath, 6, 4);
    }
}

void OnRemoveUser(HWND hDlg)
{
    char user[256] = {};
    if (!GetSelectedUser(hDlg, user, sizeof(user)))
    {
        MessageBoxA(hDlg, "Select a user to remove.", "Token Manager", MB_OK | MB_ICONWARNING);
        return;
    }

    char msg[300] = {};
    sprintf_s(msg, "Remove user '%s'? This deletes their TOTP secret, SCRAM verifier and pending session token.", user);
    if (MessageBoxA(hDlg, msg, "Token Manager", MB_YESNO | MB_ICONQUESTION) != IDYES)
        return;

    TokenCore::DeleteTotpSecret(user);
    TokenCore::DeleteProtectedValue("SCRAM", user);
    TokenCore::DeleteProtectedValue("SessionTokens", user);
    TokenCore::DeleteProtectedValue("TotpLastStep", user);
    SetDlgItemTextA(hDlg, IDC_TV_URI, "");
    UpdateFactorsLabel(hDlg, "");
    RefreshUserList(hDlg);
}

void OnSetPassword(HWND hDlg)
{
    char user[256] = {};
    if (!GetSelectedUser(hDlg, user, sizeof(user)))
    {
        MessageBoxA(hDlg, "Select a user first.", "Token Manager", MB_OK | MB_ICONWARNING);
        return;
    }

    char password[256] = {};
    GetDlgItemTextA(hDlg, IDC_TV_PASSWORD, password, sizeof(password));
    if (!password[0])
    {
        MessageBoxA(hDlg, "Enter a password in the Password field.", "Token Manager", MB_OK | MB_ICONWARNING);
        return;
    }

    ScramSha256::Verifier v;
    if (!ScramSha256::MakeVerifier(password, v) ||
        !TokenCore::WriteProtectedValue("SCRAM", user, ScramSha256::VerifierToString(v)))
    {
        MessageBoxA(hDlg, "Failed to store the SCRAM verifier.", "Token Manager", MB_OK | MB_ICONERROR);
        return;
    }

    // Never keep the plaintext around longer than needed.
    SecureZeroMemory(password, sizeof(password));
    SetDlgItemTextA(hDlg, IDC_TV_PASSWORD, "");

    MessageBoxA(hDlg, "SCRAM password stored. The plaintext is not saved — only the verifier.",
                "Token Manager", MB_OK | MB_ICONINFORMATION);
    RefreshUserList(hDlg);
    UpdateFactorsLabel(hDlg, user);
}

void OnTestPassword(HWND hDlg)
{
    char user[256] = {};
    if (!GetSelectedUser(hDlg, user, sizeof(user)))
    {
        MessageBoxA(hDlg, "Select a user first.", "Token Manager", MB_OK | MB_ICONWARNING);
        return;
    }

    std::string text;
    ScramSha256::Verifier v;
    if (!TokenCore::ReadProtectedValue("SCRAM", user, text) ||
        !ScramSha256::ParseVerifier(text, v))
    {
        MessageBoxA(hDlg, "No SCRAM credential stored for this user.", "Token Manager", MB_OK | MB_ICONWARNING);
        return;
    }

    char password[256] = {};
    GetDlgItemTextA(hDlg, IDC_TV_PASSWORD, password, sizeof(password));
    if (!password[0])
    {
        MessageBoxA(hDlg, "Enter the password to test in the Password field.", "Token Manager", MB_OK | MB_ICONWARNING);
        return;
    }

    bool ok = ScramSha256::CheckPassword(password, v);
    SecureZeroMemory(password, sizeof(password));
    SetDlgItemTextA(hDlg, IDC_TV_PASSWORD, "");

    MessageBoxA(hDlg, ok ? "Password matches the stored verifier."
                         : "Password does NOT match.",
                "Token Manager", ok ? MB_OK | MB_ICONINFORMATION
                                    : MB_OK | MB_ICONWARNING);
}

void OnClearPassword(HWND hDlg)
{
    char user[256] = {};
    if (!GetSelectedUser(hDlg, user, sizeof(user)))
    {
        MessageBoxA(hDlg, "Select a user first.", "Token Manager", MB_OK | MB_ICONWARNING);
        return;
    }

    char msg[300] = {};
    sprintf_s(msg, "Remove the SCRAM password for '%s'? SASL authentication will stop working for this user.", user);
    if (MessageBoxA(hDlg, msg, "Token Manager", MB_YESNO | MB_ICONQUESTION) != IDYES)
        return;

    TokenCore::DeleteProtectedValue("SCRAM", user);
    RefreshUserList(hDlg);
    UpdateFactorsLabel(hDlg, user);
}

void OnClearTotp(HWND hDlg)
{
    char user[256] = {};
    if (!GetSelectedUser(hDlg, user, sizeof(user)))
    {
        MessageBoxA(hDlg, "Select a user first.", "Token Manager", MB_OK | MB_ICONWARNING);
        return;
    }

    std::vector<std::string> users = TokenCore::ListTotpUsers();
    if (std::find(users.begin(), users.end(), user) == users.end())
    {
        MessageBoxA(hDlg, "No TOTP credential is stored for this user.", "Token Manager", MB_OK | MB_ICONWARNING);
        return;
    }

    char msg[300] = {};
    sprintf_s(msg, "Remove only the TOTP credential for '%s'? SCRAM and session credentials are preserved.", user);
    if (MessageBoxA(hDlg, msg, "Token Manager", MB_YESNO | MB_ICONQUESTION) != IDYES)
        return;

    if (!TokenCore::DeleteTotpSecret(user))
    {
        MessageBoxA(hDlg, "Failed to remove the TOTP credential.", "Token Manager", MB_OK | MB_ICONERROR);
        return;
    }

    SetDlgItemTextA(hDlg, IDC_TV_URI, "");
    RefreshUserList(hDlg);

    HWND hList = GetDlgItem(hDlg, IDC_TV_USER_LIST);
    int count = (int)SendMessageA(hList, LB_GETCOUNT, 0, 0);
    for (int i = 0; i < count; ++i)
    {
        char item[256] = {};
        SendMessageA(hList, LB_GETTEXT, i, (LPARAM)item);
        StripListName(item);
        if (strcmp(item, user) == 0)
        {
            SendMessageA(hList, LB_SETCURSEL, i, 0);
            break;
        }
    }
    UpdateFactorsLabel(hDlg, user);
}

void OnResetUser(HWND hDlg)
{
    char user[256] = {};
    if (!GetSelectedUser(hDlg, user, sizeof(user)))
    {
        MessageBoxA(hDlg, "Select a user to reset.", "Token Manager", MB_OK | MB_ICONWARNING);
        return;
    }

    char msg[300] = {};
    sprintf_s(msg, "Reset the secret for '%s'? The old QR code will stop working.", user);
    if (MessageBoxA(hDlg, msg, "Token Manager", MB_YESNO | MB_ICONQUESTION) != IDYES)
        return;

    std::string secret;
    if (!TokenCore::GenerateRandomBase32Secret(secret))
    {
        MessageBoxA(hDlg, "Failed to generate a new secret.", "Token Manager", MB_OK | MB_ICONERROR);
        return;
    }

    if (!TokenCore::WriteTotpSecret(user, secret))
    {
        MessageBoxA(hDlg, "Failed to save the new secret.", "Token Manager", MB_OK | MB_ICONERROR);
        return;
    }

    UpdateUriAndQr(hDlg, user, secret);

    char tempPath[MAX_PATH] = {};
    if (GetTempPathA(MAX_PATH, tempPath))
    {
        char bmpPath[MAX_PATH] = {};
        sprintf_s(bmpPath, "%s%s_uvnc_totp.bmp", tempPath, user);
        TokenCore::WriteQrBmpToFile(TokenCore::BuildOtpAuthUri(user, secret), bmpPath, 6, 4);
    }
}

void OnShowQr(HWND hDlg)
{
    char user[256] = {};
    if (!GetSelectedUser(hDlg, user, sizeof(user)))
    {
        MessageBoxA(hDlg, "Select a user to show the QR code.", "Token Manager", MB_OK | MB_ICONWARNING);
        return;
    }

    std::string secret;
    if (!TokenCore::ReadTotpSecret(user, secret) || secret.empty())
    {
        MessageBoxA(hDlg, "No stored secret found for this user.", "Token Manager", MB_OK | MB_ICONWARNING);
        return;
    }

    UpdateUriAndQr(hDlg, user, secret);
}

INT_PTR CALLBACK TokenManagerDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_INITDIALOG:
    {
        // Title-bar icon: the UltraVNC icon embedded in this DLL.
        HICON hIcon = (HICON)LoadImageW(g_hInst, MAKEINTRESOURCEW(IDI_TOKENMANAGER),
                                      IMAGE_ICON, GetSystemMetrics(SM_CXSMICON),
                                      GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);
        if (hIcon)
            SendMessage(hDlg, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
        HICON hIconBig = (HICON)LoadImageW(g_hInst, MAKEINTRESOURCEW(IDI_TOKENMANAGER),
                                         IMAGE_ICON, GetSystemMetrics(SM_CXICON),
                                         GetSystemMetrics(SM_CYICON), LR_DEFAULTCOLOR);
        if (hIconBig)
            SendMessage(hDlg, WM_SETICON, ICON_BIG, (LPARAM)hIconBig);
        RefreshUserList(hDlg);
        SetDlgItemTextA(hDlg, IDC_TV_URI, "");
        return (INT_PTR)TRUE;
    }

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDC_TV_USER_LIST:
            if (HIWORD(wParam) == LBN_SELCHANGE)
                OnUserSelectionChanged(hDlg);
            return (INT_PTR)TRUE;

        case IDC_TV_ADD:
            OnAddUser(hDlg);
            return (INT_PTR)TRUE;

        case IDC_TV_REMOVE:
            OnRemoveUser(hDlg);
            return (INT_PTR)TRUE;

        case IDC_TV_RESET:
            OnResetUser(hDlg);
            return (INT_PTR)TRUE;

        case IDC_TV_SHOWQR:
            OnShowQr(hDlg);
            return (INT_PTR)TRUE;

        case IDC_TV_DELTOTP:
            OnClearTotp(hDlg);
            return (INT_PTR)TRUE;

        case IDC_TV_SETPASS:
            OnSetPassword(hDlg);
            return (INT_PTR)TRUE;

        case IDC_TV_TESTPASS:
            OnTestPassword(hDlg);
            return (INT_PTR)TRUE;

        case IDC_TV_DELPASS:
            OnClearPassword(hDlg);
            return (INT_PTR)TRUE;

        case IDOK:
        case IDCANCEL:
        {
            if (g_hQrBitmap)
            {
                DeleteObject(g_hQrBitmap);
                g_hQrBitmap = NULL;
            }
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        }
        break;
    }

    return (INT_PTR)FALSE;
}

// ---------------------------------------------------------------------------
// Session-token helpers
// A session token remains pending until it is used or replaced. DPAPI machine
// protection lets a restarted service desktop process validate it.
// ---------------------------------------------------------------------------
constexpr const char* kSessionSection = "SessionTokens";
constexpr const char* kScramSection = "SCRAM"; // per-user "iter:saltB64:skB64:svkB64"
constexpr uint64_t kSessionTokenLifetimeSeconds = 12 * 60 * 60;
std::mutex g_sessionMutex;

std::string GenerateSessionTokenString()
{
    static const char alphabet[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    const size_t tokenLen = 64;
    const size_t alphabetLen = sizeof(alphabet) - 1;

    std::vector<BYTE> randBytes;
    randBytes.resize(tokenLen);
    BCRYPT_ALG_HANDLE hAlg = NULL;
    if (!BCRYPT_SUCCESS(BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_RNG_ALGORITHM, NULL, 0)))
        return std::string();
    NTSTATUS status = BCryptGenRandom(hAlg, randBytes.data(), static_cast<ULONG>(randBytes.size()), 0);
    BCryptCloseAlgorithmProvider(hAlg, 0);
    if (!BCRYPT_SUCCESS(status))
        return std::string();

    std::string token;
    token.reserve(tokenLen);
    for (BYTE b : randBytes)
        token.push_back(alphabet[b % alphabetLen]);
    return token;
}

bool ValidateSessionToken(const char* user, const char* token)
{
    if (!user || !token)
        return false;

    std::lock_guard<std::mutex> lock(g_sessionMutex);
    std::string record;
    if (!TokenCore::ReadProtectedValue(kSessionSection, user, record))
        return false;

    size_t separator = record.rfind('|');
    if (separator == std::string::npos)
        return false;
    std::string storedToken = record.substr(0, separator);
    uint64_t issued = _strtoui64(record.c_str() + separator + 1, nullptr, 10);
    uint64_t now = currentUnixTime();
    if (issued == 0 || now < issued || now - issued > kSessionTokenLifetimeSeconds)
    {
        TokenCore::DeleteProtectedValue(kSessionSection, user);
        return false;
    }

    size_t tokenLen = strlen(token);
    if (storedToken.size() != tokenLen)
        return false;
    unsigned char diff = 0;
    for (size_t i = 0; i < tokenLen; ++i)
        diff |= static_cast<unsigned char>(storedToken[i] ^ token[i]);
    if (diff != 0)
        return false;

    TokenCore::DeleteProtectedValue(kSessionSection, user);
    return true;
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// DLL exports
// ---------------------------------------------------------------------------
extern "C" __declspec(dllexport) int __cdecl validate_token(const char* user, const char* token)
{
    if (!user || !token)
        return 0;

    size_t len = strlen(token);

    // TOTP codes (6-8 digits) are checked against stored per-user secrets.
    if (len >= 6 && len <= 8 && isAllDigits(token))
        return validateTotp(user, token) ? 1 : 0;

    // 64-character server-issued session tokens allow reconnect without a new TOTP.
    if (len == 64)
        return ValidateSessionToken(user, token) ? 1 : 0;

    return 0;
}

extern "C" __declspec(dllexport) int __cdecl issue_session_token(const char* user, char* outBuf, size_t outBufSize)
{
    if (!user || !outBuf || outBufSize < 65)
        return 0;

    std::string token = GenerateSessionTokenString();
    if (token.empty())
        return 0;

    {
        std::lock_guard<std::mutex> lock(g_sessionMutex);
        std::string record = token + "|" + std::to_string(currentUnixTime());
        if (!TokenCore::WriteProtectedValue(kSessionSection, user, record))
            return 0;
    }

    strncpy_s(outBuf, outBufSize, token.c_str(), _TRUNCATE);
    return 1;
}

// Return the stored SCRAM-SHA-256 verifier for SASL subtypes 263/264.
// The verifier string is key material: a stolen verifier lets an attacker
// complete the server side but not recover the password, matching the
// security model of PostgreSQL/pg_authid.
extern "C" __declspec(dllexport) int __cdecl sasl_lookup(const char* user, char* outBuf, size_t outBufSize)
{
    if (!user || !*user || !outBuf || outBufSize == 0)
        return 0;

    std::string verifier;
    {
        std::lock_guard<std::mutex> lock(g_sessionMutex);
        if (!TokenCore::ReadProtectedValue(kScramSection, user, verifier) ||
            verifier.empty())
            return 0;
    }
    if (verifier.size() >= outBufSize)
        return 0;
    memcpy(outBuf, verifier.data(), verifier.size());
    outBuf[verifier.size()] = '\0';
    return 1;
}

extern "C" __declspec(dllexport) int __cdecl tv_get_capabilities(void)
{
    return TV_CAP_SESSION_TOKEN | TV_CAP_MANAGEMENT_UI | TV_CAP_SASL;
}

extern "C" __declspec(dllexport) int __cdecl tv_has_management_ui(void)
{
    return 1;
}

extern "C" __declspec(dllexport) int __cdecl tv_show_management_ui(HWND hwndParent)
{
    g_hQrBitmap = NULL;
    INT_PTR ret = DialogBoxParamA(g_hInst, MAKEINTRESOURCEA(IDD_TVMANAGER), hwndParent,
                                   TokenManagerDlgProc, 0);
    if (g_hQrBitmap)
    {
        DeleteObject(g_hQrBitmap);
        g_hQrBitmap = NULL;
    }
    return (ret == IDOK || ret == IDCANCEL) ? 1 : 0;
}

extern "C" __declspec(dllexport) int __cdecl tv_get_last_error(char* buf, size_t bufSize)
{
    if (buf && bufSize > 0)
        buf[0] = '\0';
    return 0;
}

// ---------------------------------------------------------------------------
// DllMain
// ---------------------------------------------------------------------------
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        g_hInst = (HINSTANCE)hModule;
        TokenCore::SetSecretStorePathsFromModuleHandle(hModule);
        DisableThreadLibraryCalls(hModule);
        break;

    case DLL_PROCESS_DETACH:
        if (g_hQrBitmap)
        {
            DeleteObject(g_hQrBitmap);
            g_hQrBitmap = NULL;
        }
        break;
    }
    return TRUE;
}
