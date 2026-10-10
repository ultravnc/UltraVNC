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
// Supports two credential types:
//   1. TOTP (RFC 6238) codes from authenticator apps such as Microsoft
//      Authenticator, Google Authenticator, etc.
//   2. Server-issued 64-character session tokens for reconnects.
//
// Exports:
//   validate_token(user, token)
//   tv_has_management_ui()
//   tv_show_management_ui(HWND parent)
//
// TOTP secrets are read from Windows Credential Manager first (target name
// "UltraVNC_TOTP_<user>"), and fall back to a DPAPI-encrypted INI file next to
// this DLL (<this DLL>.totp.ini) for service accounts such as SYSTEM whose
// Credential Manager vault is different from the interactive admin's vault.

#include "TokenCore.h"
#include "TokenValidatorPlugin.h"
#include "TokenRes.h"

#include <windows.h>
#include <bcrypt.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <vector>
#include <string>
#include <unordered_map>
#include <mutex>

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
    for (int64_t delta = -1; delta <= 1; ++delta)
    {
        if (hotp(secret.data(), secret.size(), t + delta) == expected)
            return true;
    }
    return false;
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

    std::vector<std::string> users = TokenCore::ListTotpUsers();
    for (const auto& u : users)
    {
        int idx = (int)SendMessageA(hList, LB_ADDSTRING, 0, (LPARAM)u.c_str());
        if (idx != LB_ERR && strcmp(u.c_str(), prevUser) == 0)
            SendMessageA(hList, LB_SETCURSEL, idx, 0);
    }
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
        return;
    }

    char user[256] = {};
    SendMessageA(hList, LB_GETTEXT, idx, (LPARAM)user);

    // Strip the read-only fallback marker if selected.
    size_t len = strlen(user);
    if (len > 20 && strcmp(user + len - 20, " (static fallback)") == 0)
        user[len - 20] = '\0';

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
    size_t len = strlen(user);
    if (len > 20 && strcmp(user + len - 20, " (static fallback)") == 0)
        return false; // read-only static fallback

    return true;
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
    sprintf_s(msg, "Remove user '%s'? This will invalidate their authenticator.", user);
    if (MessageBoxA(hDlg, msg, "Token Manager", MB_YESNO | MB_ICONQUESTION) != IDYES)
        return;

    TokenCore::DeleteTotpSecret(user);
    SetDlgItemTextA(hDlg, IDC_TV_URI, "");
    RefreshUserList(hDlg);
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
// Session-token helpers (in-memory, server-side only)
// A session token is issued after a successful TOTP authentication and remains
// valid for 60 seconds after each disconnect / validation. Every successful
// reuse extends the lifetime by another 60 seconds.
// ---------------------------------------------------------------------------
struct SessionEntry
{
    std::string user;
    time_t expiresAt;
};

std::mutex g_sessionMutex;
std::unordered_map<std::string, SessionEntry> g_sessions;

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
    auto it = g_sessions.find(token);
    if (it == g_sessions.end())
        return false;

    time_t now = time(nullptr);
    if (now > it->second.expiresAt)
    {
        g_sessions.erase(it);
        return false;
    }

    if (it->second.user != user)
        return false;

    // Extend rolling lifetime.
    it->second.expiresAt = now + 60;
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

        // Only one active session token per user: revoke any previous token.
        for (auto it = g_sessions.begin(); it != g_sessions.end();)
        {
            if (it->second.user == user)
                it = g_sessions.erase(it);
            else
                ++it;
        }

        SessionEntry entry;
        entry.user = user;
        entry.expiresAt = time(nullptr) + 60;
        g_sessions[token] = entry;
    }

    strncpy_s(outBuf, outBufSize, token.c_str(), _TRUNCATE);
    return 1;
}

extern "C" __declspec(dllexport) int __cdecl tv_get_capabilities(void)
{
    return TV_CAP_SESSION_TOKEN | TV_CAP_MANAGEMENT_UI;
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
