// TokenSetup.exe
//
// Registers a user for UltraVNC VeNCrypt TOTP authentication.
//
// Usage:
//   TokenSetup.exe <username> [base32-secret]
//
// If the secret is omitted, a random 160-bit base32 secret is generated.
// The tool stores the secret in Windows Credential Manager and in a
// DPAPI-encrypted INI file next to TokenValidator.dll/TokenSetup.exe so the
// WinVNC token validation DLL can read it even when running as SYSTEM.
// It also emits an otpauth:// provisioning URI and a QR-code BMP image that
// can be scanned by Microsoft Authenticator, Google Authenticator, Authy, etc.

#include "TokenCore.h"

#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <string>

static bool SetStoragePathsFromExeDir()
{
    char exePath[MAX_PATH] = {};
    if (!GetModuleFileNameA(NULL, exePath, MAX_PATH))
        return false;

    char* lastSlash = strrchr(exePath, '\\');
    if (!lastSlash)
        return false;

    // Prefer the matching TokenValidator DLL next to us so the DLL and this
    // tool share the same INI fallback.
    char baseName[MAX_PATH] = {};
    strncpy_s(baseName, sizeof(baseName), exePath, lastSlash - exePath + 1);

    for (const char* dllName : { "TokenValidator.dll", "TokenValidator32.dll" })
    {
        char candidate[MAX_PATH] = {};
        sprintf_s(candidate, "%s%s", baseName, dllName);
        if (GetFileAttributesA(candidate) != INVALID_FILE_ATTRIBUTES)
        {
            TokenCore::SetSecretStorePathsFromDllPath(candidate);
            return true;
        }
    }

    // No DLL found: use the INI next to this executable.
    TokenCore::SetSecretStorePathsFromDllPath(exePath);
    return true;
}

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s <username> [base32-secret]\n", argv[0]);
        fprintf(stderr, "If the base32 secret is omitted, a random 160-bit secret is generated.\n");
        return 1;
    }

    SetStoragePathsFromExeDir();

    const char* user = argv[1];
    std::string secretBase32;

    if (argc >= 3)
    {
        secretBase32 = argv[2];
        std::string cleaned;
        for (char c : secretBase32)
        {
            if (c == ' ' || c == '-')
                continue;
            cleaned.push_back(static_cast<char>(toupper(static_cast<unsigned char>(c))));
        }
        secretBase32 = cleaned;
    }
    else
    {
        if (!TokenCore::GenerateRandomBase32Secret(secretBase32))
        {
            fprintf(stderr, "Failed to generate random secret\n");
            return 1;
        }
    }

    if (!TokenCore::ValidateBase32(secretBase32))
    {
        fprintf(stderr, "Invalid base32 secret (length must be a multiple of 8, A-Z2-7)\n");
        return 1;
    }

    if (!TokenCore::WriteTotpSecret(user, secretBase32))
    {
        fprintf(stderr, "Warning: failed to store secret (Credential Manager or INI fallback)\n");
    }

    std::string uri = TokenCore::BuildOtpAuthUri(user, secretBase32);

    // Write QR image next to this executable.
    char bmpPath[MAX_PATH] = {};
    if (GetModuleFileNameA(NULL, bmpPath, MAX_PATH))
    {
        char* dot = strrchr(bmpPath, '.');
        if (dot)
        {
            char suffix[256] = {};
            sprintf_s(suffix, "_%s_totp.bmp", user);
            strcpy_s(dot, MAX_PATH - (dot - bmpPath), suffix);
        }
    }
    if (!bmpPath[0])
        sprintf_s(bmpPath, "%s_totp.bmp", user);

    if (!TokenCore::WriteQrBmpToFile(uri, bmpPath, 6, 4))
    {
        fprintf(stderr, "Warning: failed to write QR code image\n");
        bmpPath[0] = '\0';
    }

    printf("User: %s\n", user);
    printf("Base32 secret: %s\n", secretBase32.c_str());
    printf("Provisioning URI: %s\n", uri.c_str());
    if (bmpPath[0])
        printf("QR code image: %s\n", bmpPath);
    printf("Credential Manager target: UltraVNC_TOTP_%s\n", user);
    printf("INI fallback: %s\n", TokenCore::GetSecretStoreIniPath().c_str());
    return 0;
}
