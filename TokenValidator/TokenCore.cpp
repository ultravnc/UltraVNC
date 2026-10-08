// TokenCore.cpp - shared TOTP helpers for UltraVNC TokenValidator

#include "TokenCore.h"
#include "qrcodegen.hpp"

#include <windows.h>
#include <bcrypt.h>
#include <wincred.h>
#include <dpapi.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <accctrl.h>
#include <aclapi.h>

#pragma comment(lib, "bcrypt.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "crypt32.lib")

namespace TokenCore {

namespace {

constexpr const char* kCredPrefix = "UltraVNC_TOTP_";
constexpr size_t kCredPrefixLen = 14; // strlen("UltraVNC_TOTP_")

char g_iniPath[MAX_PATH] = {};

bool SetIniPath(const char* modulePath)
{
    if (!modulePath || !modulePath[0])
        return false;

    const char* lastSlash = strrchr(modulePath, '\\');
    if (!lastSlash)
        return false;

    size_t dirLen = lastSlash - modulePath + 1;
    if (dirLen + 16 >= MAX_PATH)
        return false;

    memcpy(g_iniPath, modulePath, dirLen);
    strcpy_s(g_iniPath + dirLen, MAX_PATH - dirLen, "TokenValidator.totp.ini");
    return true;
}

const char* GetIniPath()
{
    if (g_iniPath[0])
        return g_iniPath;

    char path[MAX_PATH] = {};
    if (GetModuleFileNameA(NULL, path, MAX_PATH))
        SetIniPath(path);

    return g_iniPath;
}

std::string UrlEncode(const std::string& s)
{
    std::string out;
    for (unsigned char c : s)
    {
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') ||
            c == '-' || c == '_' || c == '.' || c == '~')
        {
            out.push_back(static_cast<char>(c));
        }
        else
        {
            char buf[4] = {};
            sprintf_s(buf, "%%%02X", c);
            out += buf;
        }
    }
    return out;
}

std::string WcharToUtf8(const wchar_t* wide)
{
    if (!wide || !wide[0])
        return std::string();
    int len = WideCharToMultiByte(CP_UTF8, 0, wide, -1, NULL, 0, NULL, NULL);
    if (len <= 0)
        return std::string();
    std::string s(len, '\0');
    WideCharToMultiByte(CP_UTF8, 0, wide, -1, &s[0], len, NULL, NULL);
    s.resize(strlen(s.c_str()));
    return s;
}

std::string Utf8ToWcharString(const char* utf8, std::wstring& wide)
{
    wide.clear();
    if (!utf8 || !utf8[0])
        return std::string();
    int len = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, NULL, 0);
    if (len <= 0)
        return std::string();
    wide.resize(len);
    MultiByteToWideChar(CP_UTF8, 0, utf8, -1, &wide[0], len);
    wide.resize(wcslen(wide.c_str()));
    return WcharToUtf8(wide.c_str());
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// Base32 encode/decode
// ---------------------------------------------------------------------------
bool Base32Decode(const std::string& in, std::vector<BYTE>& out)
{
    static const int table[256] = {
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,26,27,28,29,30,31,-1,-1,-1,-1,-1,-1,-1,-1,
        -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,
        15,16,17,18,19,20,21,22,23,24,25,-1,-1,-1,-1,-1,
        -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,
        15,16,17,18,19,20,21,22,23,24,25,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1
    };

    out.clear();
    int buffer = 0;
    int bitsInBuffer = 0;

    for (unsigned char c : in)
    {
        if (c == '=')
            break;
        if (c == ' ' || c == '-')
            continue;
        if (c >= 256)
            return false;
        int val = table[c];
        if (val < 0)
            return false;
        buffer = (buffer << 5) | val;
        bitsInBuffer += 5;
        if (bitsInBuffer >= 8)
        {
            bitsInBuffer -= 8;
            out.push_back(static_cast<BYTE>(buffer >> bitsInBuffer));
            buffer &= (1 << bitsInBuffer) - 1;
        }
    }
    return !out.empty();
}

static std::string Base32Encode(const std::vector<BYTE>& data)
{
    static const char alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";
    std::string out;
    int buffer = 0;
    int bitsInBuffer = 0;
    for (BYTE b : data)
    {
        buffer = (buffer << 8) | b;
        bitsInBuffer += 8;
        while (bitsInBuffer >= 5)
        {
            out.push_back(alphabet[(buffer >> (bitsInBuffer - 5)) & 0x1F]);
            bitsInBuffer -= 5;
            buffer &= (1 << bitsInBuffer) - 1;
        }
    }
    if (bitsInBuffer > 0)
    {
        out.push_back(alphabet[(buffer << (5 - bitsInBuffer)) & 0x1F]);
    }
    return out;
}

static bool GenerateRandomBytes(std::vector<BYTE>& out, ULONG len)
{
    out.resize(len);
    BCRYPT_ALG_HANDLE hAlg = NULL;
    if (!BCRYPT_SUCCESS(BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_RNG_ALGORITHM, NULL, 0)))
        return false;
    NTSTATUS status = BCryptGenRandom(hAlg, out.data(), len, 0);
    BCryptCloseAlgorithmProvider(hAlg, 0);
    return BCRYPT_SUCCESS(status);
}

bool GenerateRandomBase32Secret(std::string& out, size_t bits)
{
    if (bits % 8 != 0 || bits == 0)
        return false;
    std::vector<BYTE> randBytes;
    if (!GenerateRandomBytes(randBytes, static_cast<ULONG>(bits / 8)))
        return false;
    out = Base32Encode(randBytes);
    return true;
}

bool ValidateBase32(const std::string& s)
{
    if (s.empty() || (s.size() % 8) != 0)
        return false;
    for (char c : s)
    {
        if (c >= 'A' && c <= 'Z') continue;
        if (c >= '2' && c <= '7') continue;
        return false;
    }
    return true;
}

// ---------------------------------------------------------------------------
// Storage path setup
// ---------------------------------------------------------------------------
bool SetSecretStorePathsFromDllPath(const char* dllPath)
{
    return SetIniPath(dllPath);
}

bool SetSecretStorePathsFromModuleHandle(HMODULE hModule)
{
    char path[MAX_PATH] = {};
    if (!GetModuleFileNameA(hModule, path, MAX_PATH))
        return false;
    return SetIniPath(path);
}

std::string GetSecretStoreIniPath()
{
    const char* path = GetIniPath();
    return path ? std::string(path) : std::string();
}

// ---------------------------------------------------------------------------
// Credential Manager + DPAPI INI helpers
// ---------------------------------------------------------------------------
static bool WriteCredential(const char* user, const std::string& secretUtf8)
{
    std::wstring wideUser;
    Utf8ToWcharString(user, wideUser);
    if (wideUser.empty())
        return false;

    wchar_t targetName[256] = {};
    swprintf_s(targetName, L"%hs%ls", kCredPrefix, wideUser.c_str());

    CREDENTIALW cred = {};
    cred.Type = CRED_TYPE_GENERIC;
    cred.TargetName = targetName;
    cred.CredentialBlobSize = static_cast<DWORD>(secretUtf8.size());
    cred.CredentialBlob = const_cast<BYTE*>(reinterpret_cast<const BYTE*>(secretUtf8.data()));
    cred.Persist = CRED_PERSIST_LOCAL_MACHINE;
    cred.UserName = const_cast<LPWSTR>(wideUser.c_str());

    return CredWriteW(&cred, 0) == TRUE;
}

static bool ReadCredential(const char* user, std::string& secretOut)
{
    std::wstring wideUser;
    Utf8ToWcharString(user, wideUser);
    if (wideUser.empty())
        return false;

    wchar_t targetName[256] = {};
    swprintf_s(targetName, L"%hs%ls", kCredPrefix, wideUser.c_str());

    PCREDENTIALW cred = NULL;
    if (!CredReadW(targetName, CRED_TYPE_GENERIC, 0, &cred) || !cred)
        return false;

    if (cred->CredentialBlob && cred->CredentialBlobSize > 0)
    {
        secretOut.assign(reinterpret_cast<const char*>(cred->CredentialBlob), cred->CredentialBlobSize);
    }

    CredFree(cred);
    return !secretOut.empty();
}

static bool DeleteCredential(const char* user)
{
    std::wstring wideUser;
    Utf8ToWcharString(user, wideUser);
    if (wideUser.empty())
        return false;

    wchar_t targetName[256] = {};
    swprintf_s(targetName, L"%hs%ls", kCredPrefix, wideUser.c_str());

    return CredDeleteW(targetName, CRED_TYPE_GENERIC, 0) == TRUE;
}

static bool SetSecureFileAcl(const char* path)
{
    HANDLE hToken = NULL;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
        return false;

    DWORD userInfoLen = 0;
    GetTokenInformation(hToken, TokenUser, NULL, 0, &userInfoLen);
    if (userInfoLen == 0)
    {
        CloseHandle(hToken);
        return false;
    }
    std::vector<BYTE> userInfo(userInfoLen);
    PTOKEN_USER pUser = reinterpret_cast<PTOKEN_USER>(userInfo.data());
    if (!GetTokenInformation(hToken, TokenUser, pUser, userInfoLen, &userInfoLen) ||
        !IsValidSid(pUser->User.Sid))
    {
        CloseHandle(hToken);
        return false;
    }
    PSID userSid = pUser->User.Sid;

    std::vector<BYTE> systemSid(SECURITY_MAX_SID_SIZE);
    std::vector<BYTE> adminSid(SECURITY_MAX_SID_SIZE);
    DWORD sysLen = SECURITY_MAX_SID_SIZE, adminLen = SECURITY_MAX_SID_SIZE;
    BOOL sidsOk = CreateWellKnownSid(WinLocalSystemSid, NULL, systemSid.data(), &sysLen) &&
                  CreateWellKnownSid(WinBuiltinAdministratorsSid, NULL, adminSid.data(), &adminLen);
    if (!sidsOk)
    {
        CloseHandle(hToken);
        return false;
    }

    EXPLICIT_ACCESS_A ea[3] = {};

    ea[0].grfAccessPermissions = GENERIC_ALL;
    ea[0].grfAccessMode = SET_ACCESS;
    ea[0].grfInheritance = NO_INHERITANCE;
    ea[0].Trustee.TrusteeForm = TRUSTEE_IS_SID;
    ea[0].Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
    ea[0].Trustee.ptstrName = reinterpret_cast<LPSTR>(systemSid.data());

    ea[1].grfAccessPermissions = GENERIC_ALL;
    ea[1].grfAccessMode = SET_ACCESS;
    ea[1].grfInheritance = NO_INHERITANCE;
    ea[1].Trustee.TrusteeForm = TRUSTEE_IS_SID;
    ea[1].Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
    ea[1].Trustee.ptstrName = reinterpret_cast<LPSTR>(adminSid.data());

    ea[2].grfAccessPermissions = GENERIC_ALL;
    ea[2].grfAccessMode = SET_ACCESS;
    ea[2].grfInheritance = NO_INHERITANCE;
    ea[2].Trustee.TrusteeForm = TRUSTEE_IS_SID;
    ea[2].Trustee.TrusteeType = TRUSTEE_IS_USER;
    ea[2].Trustee.ptstrName = reinterpret_cast<LPSTR>(userSid);

    PACL pAcl = NULL;
    if (SetEntriesInAclA(3, ea, NULL, &pAcl) != ERROR_SUCCESS)
    {
        CloseHandle(hToken);
        return false;
    }

    PSECURITY_DESCRIPTOR pSd = reinterpret_cast<PSECURITY_DESCRIPTOR>(LocalAlloc(LPTR, SECURITY_DESCRIPTOR_MIN_LENGTH));
    if (!pSd || !InitializeSecurityDescriptor(pSd, SECURITY_DESCRIPTOR_REVISION) ||
        !SetSecurityDescriptorDacl(pSd, TRUE, pAcl, FALSE))
    {
        LocalFree(pAcl);
        LocalFree(pSd);
        CloseHandle(hToken);
        return false;
    }
    SetSecurityDescriptorControl(pSd, SE_DACL_PROTECTED, SE_DACL_PROTECTED);

    BOOL ok = SetFileSecurityA(path, DACL_SECURITY_INFORMATION, pSd);

    LocalFree(pAcl);
    LocalFree(pSd);
    CloseHandle(hToken);
    return ok == TRUE;
}

static bool WriteEncryptedIni(const char* user, const std::string& secretUtf8)
{
    const char* iniPath = GetIniPath();
    if (!iniPath || !iniPath[0])
        return false;

    DATA_BLOB in = { static_cast<DWORD>(secretUtf8.size()),
                     const_cast<BYTE*>(reinterpret_cast<const BYTE*>(secretUtf8.data())) };
    DATA_BLOB out = {};

    if (!CryptProtectData(&in, L"UltraVNC TOTP secret", NULL, NULL, NULL,
                          CRYPTPROTECT_LOCAL_MACHINE, &out))
    {
        return false;
    }

    DWORD base64Len = 0;
    if (!CryptBinaryToStringA(out.pbData, out.cbData, CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, NULL, &base64Len))
    {
        LocalFree(out.pbData);
        return false;
    }

    std::vector<char> base64(base64Len);
    if (!CryptBinaryToStringA(out.pbData, out.cbData, CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, base64.data(), &base64Len))
    {
        LocalFree(out.pbData);
        return false;
    }
    LocalFree(out.pbData);

    if (!WritePrivateProfileStringA("TOTP", user, base64.data(), iniPath))
        return false;

    // Harden the fallback INI so only SYSTEM, Administrators, and the current
    // user can read it. Failures here are not fatal.
    SetSecureFileAcl(iniPath);
    return true;
}

static bool ReadEncryptedIni(const char* user, std::string& secretOut)
{
    const char* iniPath = GetIniPath();
    if (!iniPath || !iniPath[0])
        return false;

    char base64[4096] = {};
    if (!GetPrivateProfileStringA("TOTP", user, "", base64, sizeof(base64), iniPath))
        return false;

    DWORD blobLen = 0;
    if (!CryptStringToBinaryA(base64, 0, CRYPT_STRING_BASE64, NULL, &blobLen, NULL, NULL) || blobLen == 0)
        return false;

    std::vector<BYTE> blob(blobLen);
    if (!CryptStringToBinaryA(base64, 0, CRYPT_STRING_BASE64, blob.data(), &blobLen, NULL, NULL))
        return false;

    DATA_BLOB in = { blobLen, blob.data() };
    DATA_BLOB out = {};
    if (!CryptUnprotectData(&in, NULL, NULL, NULL, NULL, 0, &out))
        return false;

    if (out.pbData && out.cbData > 0)
        secretOut.assign(reinterpret_cast<const char*>(out.pbData), out.cbData);

    LocalFree(out.pbData);
    return !secretOut.empty();
}

static bool DeleteEncryptedIni(const char* user)
{
    const char* iniPath = GetIniPath();
    if (!iniPath || !iniPath[0])
        return false;
    return WritePrivateProfileStringA("TOTP", user, NULL, iniPath) != FALSE;
}

// ---------------------------------------------------------------------------
// Public storage API
// ---------------------------------------------------------------------------
bool ReadTotpSecret(const std::string& user, std::string& secretOut)
{
    secretOut.clear();
    if (user.empty())
        return false;
    if (ReadCredential(user.c_str(), secretOut))
        return true;
    return ReadEncryptedIni(user.c_str(), secretOut);
}

bool WriteTotpSecret(const std::string& user, const std::string& secret)
{
    if (user.empty() || secret.empty())
        return false;

    bool ok1 = WriteCredential(user.c_str(), secret);
    bool ok2 = WriteEncryptedIni(user.c_str(), secret);
    return ok1 || ok2;
}

bool DeleteTotpSecret(const std::string& user)
{
    if (user.empty())
        return false;
    bool ok1 = DeleteCredential(user.c_str());
    bool ok2 = DeleteEncryptedIni(user.c_str());
    return ok1 || ok2;
}

std::vector<std::string> ListTotpUsers()
{
    std::vector<std::string> users;

    // Credential Manager
    DWORD count = 0;
    PCREDENTIALW* creds = NULL;
    if (CredEnumerateW(NULL, 0, &count, &creds) && creds)
    {
        for (DWORD i = 0; i < count; ++i)
        {
            if (!creds[i] || !creds[i]->TargetName)
                continue;
            std::string target = WcharToUtf8(creds[i]->TargetName);
            if (target.find(kCredPrefix) == 0)
            {
                users.push_back(target.substr(kCredPrefixLen));
            }
        }
        CredFree(creds);
    }

    // INI fallback
    const char* iniPath = GetIniPath();
    if (iniPath && iniPath[0])
    {
        char section[4096] = {};
        if (GetPrivateProfileSectionA("TOTP", section, sizeof(section), iniPath) > 0)
        {
            for (const char* p = section; *p; p += strlen(p) + 1)
            {
                std::string entry(p);
                size_t eq = entry.find('=');
                std::string user = (eq == std::string::npos) ? entry : entry.substr(0, eq);
                if (!user.empty() &&
                    std::find(users.begin(), users.end(), user) == users.end())
                {
                    users.push_back(user);
                }
            }
        }
    }

    return users;
}

// ---------------------------------------------------------------------------
// TOTP URI
// ---------------------------------------------------------------------------
std::string BuildOtpAuthUri(const std::string& user,
                            const std::string& secret,
                            const std::string& issuer)
{
    std::string uri = "otpauth://totp/";
    uri += UrlEncode(issuer);
    uri += ":";
    uri += UrlEncode(user);
    uri += "?secret=";
    uri += secret;
    uri += "&issuer=";
    uri += UrlEncode(issuer);
    return uri;
}

// ---------------------------------------------------------------------------
// QR rendering
// ---------------------------------------------------------------------------
namespace {

#pragma pack(push, 1)
struct BMPFILEHEADER
{
    WORD  bfType;
    DWORD bfSize;
    WORD  bfReserved1;
    WORD  bfReserved2;
    DWORD bfOffBits;
};
struct BMPINFOHEADER
{
    DWORD biSize;
    LONG  biWidth;
    LONG  biHeight;
    WORD  biPlanes;
    WORD  biBitCount;
    DWORD biCompression;
    DWORD biSizeImage;
    LONG  biXPelsPerMeter;
    LONG  biYPelsPerMeter;
    DWORD biClrUsed;
    DWORD biClrImportant;
};
#pragma pack(pop)

static bool RenderQrPixels(const std::string& text,
                             int moduleScale,
                             int quietModules,
                             LONG& widthOut,
                             LONG& heightOut,
                             int& rowSizeOut,
                             std::vector<BYTE>& pixelsOut)
{
    try
    {
        qrcodegen::QrCode qr = qrcodegen::QrCode::encodeText(text.c_str(), qrcodegen::QrCode::Ecc::MEDIUM);
        int modules = qr.getSize();
        int imgSize = modules + 2 * quietModules;
        LONG width = imgSize * moduleScale;
        LONG height = width;
        int rowSize = ((width * 3 + 3) & ~3);
        int pixelArraySize = rowSize * height;

        pixelsOut.assign(pixelArraySize, 0xFF);

        auto setPixel = [&](int x, int y)
        {
            int row = (height - 1 - y);
            BYTE* p = pixelsOut.data() + row * rowSize + x * 3;
            p[0] = 0; p[1] = 0; p[2] = 0;
        };

        for (int qrY = -quietModules; qrY < modules + quietModules; ++qrY)
        {
            for (int qrX = -quietModules; qrX < modules + quietModules; ++qrX)
            {
                bool dark = false;
                if (qrX >= 0 && qrX < modules && qrY >= 0 && qrY < modules)
                    dark = qr.getModule(qrX, qrY);

                if (dark)
                {
                    int startX = (qrX + quietModules) * moduleScale;
                    int startY = (qrY + quietModules) * moduleScale;
                    for (int dy = 0; dy < moduleScale; ++dy)
                        for (int dx = 0; dx < moduleScale; ++dx)
                            setPixel(startX + dx, startY + dy);
                }
            }
        }

        widthOut = width;
        heightOut = height;
        rowSizeOut = rowSize;
        return true;
    }
    catch (...)
    {
        return false;
    }
}

} // anonymous namespace

bool WriteQrBmpToFile(const std::string& text,
                       const std::string& filename,
                       int moduleScale,
                       int quietModules)
{
    LONG width = 0, height = 0;
    int rowSize = 0;
    std::vector<BYTE> pixels;
    if (!RenderQrPixels(text, moduleScale, quietModules, width, height, rowSize, pixels))
        return false;

    BMPFILEHEADER fh = {};
    BMPINFOHEADER ih = {};
    fh.bfType = 0x4D42; // 'BM'
    fh.bfSize = sizeof(fh) + sizeof(ih) + static_cast<DWORD>(pixels.size());
    fh.bfOffBits = sizeof(fh) + sizeof(ih);
    ih.biSize = sizeof(ih);
    ih.biWidth = width;
    ih.biHeight = height;
    ih.biPlanes = 1;
    ih.biBitCount = 24;
    ih.biCompression = BI_RGB;
    ih.biSizeImage = static_cast<DWORD>(pixels.size());

    HANDLE hFile = CreateFileA(filename.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                               FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
        return false;

    DWORD written = 0;
    WriteFile(hFile, &fh, sizeof(fh), &written, NULL);
    WriteFile(hFile, &ih, sizeof(ih), &written, NULL);
    WriteFile(hFile, pixels.data(), static_cast<DWORD>(pixels.size()), &written, NULL);
    CloseHandle(hFile);
    return true;
}

HBITMAP RenderQrBitmap(HWND hwndReference,
                        const std::string& text,
                        int moduleScale,
                        int quietModules)
{
    LONG width = 0, height = 0;
    int rowSize = 0;
    std::vector<BYTE> pixels;
    if (!RenderQrPixels(text, moduleScale, quietModules, width, height, rowSize, pixels))
        return NULL;

    HDC hdcRef = hwndReference ? GetDC(hwndReference) : GetDC(NULL);
    if (!hdcRef)
        return NULL;

    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = height; // positive = bottom-up, matches our buffer
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 24;
    bmi.bmiHeader.biCompression = BI_RGB;
    bmi.bmiHeader.biSizeImage = static_cast<DWORD>(pixels.size());

    HBITMAP hBmp = NULL;
    HDC hdcMem = CreateCompatibleDC(hdcRef);
    if (hdcMem)
    {
        void* pBits = NULL;
        hBmp = CreateDIBSection(hdcRef, &bmi, DIB_RGB_COLORS, &pBits, NULL, 0);
        if (hBmp && pBits)
        {
            memcpy(pBits, pixels.data(), pixels.size());
        }
        DeleteDC(hdcMem);
    }

    ReleaseDC(hwndReference ? hwndReference : NULL, hdcRef);
    return hBmp;
}

} // namespace TokenCore
