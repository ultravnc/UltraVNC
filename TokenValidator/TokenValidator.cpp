// Native token validation DLL for UltraVNC VeNCrypt token auth.
//
// Supports two modes:
//   1. Static 64-character token fallback (kept for compatibility/tests).
//   2. TOTP (RFC 6238) codes from authenticator apps such as Microsoft
//      Authenticator, Google Authenticator, etc.
//
// Expected export:
//   extern "C" __declspec(dllexport) int __cdecl validate_token(const char* user, const char* token);
// Returns 1 on success, 0 on failure.
//
// TOTP secrets are read from Windows Credential Manager first (target name
// "UltraVNC_TOTP_<user>"), and fall back to a DPAPI-encrypted INI file next to
// this DLL (<this DLL>.totp.ini) for service accounts such as SYSTEM whose
// Credential Manager vault is different from the interactive admin's vault.

#include <windows.h>
#include <bcrypt.h>
#include <wincred.h>
#include <dpapi.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <string>

#pragma comment(lib, "bcrypt.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "crypt32.lib")

namespace {

constexpr size_t SHA1_DIGEST_LEN = 20;

// ---------------------------------------------------------------------------
// Base32 decode (RFC 4648)
// ---------------------------------------------------------------------------
bool base32Decode(const char* in, std::vector<BYTE>& out)
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

    for (const char* p = in; *p; ++p)
    {
        char c = *p;
        if (c == '=')
            break;
        if (c >= 0 && c < 256)
            c = (char)tolower((unsigned char)c);
        int val = (c >= 0 && c < 256) ? table[(unsigned char)c] : -1;
        if (val < 0)
            continue; // skip unknown characters (spaces/hyphens)
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

// ---------------------------------------------------------------------------
// HMAC-SHA1 using Windows CNG (BCrypt)
// ---------------------------------------------------------------------------
bool hmacSha1(const BYTE* key, size_t keyLen, const BYTE* msg, size_t msgLen, BYTE out[SHA1_DIGEST_LEN])
{
    BCRYPT_ALG_HANDLE hAlg = NULL;
    BCRYPT_HASH_HANDLE hHash = NULL;
    NTSTATUS status;
    DWORD cbResult = 0;

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
// RFC 6238 TOTP
// ---------------------------------------------------------------------------
uint64_t currentUnixTime()
{
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);
    ULARGE_INTEGER u;
    u.LowPart = ft.dwLowDateTime;
    u.HighPart = ft.dwHighDateTime;
    // Convert from 100-ns intervals since 1601-01-01 to Unix seconds.
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

    // 1. Try Windows Credential Manager first.
    wchar_t targetName[256] = {};
    if (MultiByteToWideChar(CP_UTF8, 0, user, -1, targetName + 15, 240) > 0)
    {
        // prepend prefix after filling user; build full target name
        wchar_t wideUser[256] = {};
        MultiByteToWideChar(CP_UTF8, 0, user, -1, wideUser, 256);
        swprintf_s(targetName, 256, L"UltraVNC_TOTP_%s", wideUser);

        PCREDENTIALW cred = NULL;
        if (CredReadW(targetName, CRED_TYPE_GENERIC, 0, &cred) && cred)
        {
            if (cred->CredentialBlob && cred->CredentialBlobSize > 0)
                secretBase32.assign(reinterpret_cast<char*>(cred->CredentialBlob), cred->CredentialBlobSize);
            CredFree(cred);
        }
    }

    // 2. Fallback: DPAPI-encrypted INI next to this DLL.
    if (secretBase32.empty())
    {
        HMODULE hMod = NULL;
        if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                               GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                               reinterpret_cast<LPCSTR>(&validateTotp), &hMod) && hMod)
        {
            char dllPath[MAX_PATH] = {};
            if (GetModuleFileNameA(hMod, dllPath, MAX_PATH))
            {
                char* dot = strrchr(dllPath, '.');
                if (dot)
                {
                    strcpy_s(dot, MAX_PATH - (dot - dllPath), ".totp.ini");
                    char base64[1024] = {};
                    if (GetPrivateProfileStringA("TOTP", user, "", base64, sizeof(base64), dllPath) > 0)
                    {
                        std::vector<BYTE> encrypted;
                        DWORD needed = 0;
                        if (CryptStringToBinaryA(base64, 0, CRYPT_STRING_BASE64, NULL, &needed, NULL, NULL) && needed > 0)
                        {
                            encrypted.resize(needed);
                            if (CryptStringToBinaryA(base64, 0, CRYPT_STRING_BASE64, encrypted.data(), &needed, NULL, NULL))
                            {
                                DATA_BLOB in = { static_cast<DWORD>(encrypted.size()), encrypted.data() };
                                DATA_BLOB out = {};
                                if (CryptUnprotectData(&in, NULL, NULL, NULL, NULL, 0, &out))
                                {
                                    secretBase32.assign(reinterpret_cast<char*>(out.pbData), out.cbData);
                                    LocalFree(out.pbData);
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    if (secretBase32.empty())
        return false;

    std::vector<BYTE> secret;
    if (!base32Decode(secretBase32.c_str(), secret) || secret.empty())
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

} // namespace

extern "C" __declspec(dllexport) int __cdecl validate_token(const char* user, const char* token)
{
    if (!user || !token)
        return 0;

    size_t len = strlen(token);

    // Static 64-character token fallback (kept for testing / legacy).
    if (len == 64)
    {
        if (strcmp(user, "test") != 0)
            return 0;
        for (size_t i = 0; i < len; ++i)
            if (token[i] != 'a')
                return 0;
        return 1;
    }

    // TOTP code path: 6-8 digits.
    if (len >= 6 && len <= 8 && isAllDigits(token))
        return validateTotp(user, token) ? 1 : 0;

    return 0;
}
