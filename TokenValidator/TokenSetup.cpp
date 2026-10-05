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

#include <windows.h>
#include <bcrypt.h>
#include <wincred.h>
#include <dpapi.h>
#include <stdio.h>
#include <string.h>
#include <vector>
#include <string>
#include "qrcodegen.hpp"

#pragma comment(lib, "bcrypt.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "crypt32.lib")

namespace {

// ---------------------------------------------------------------------------
// Base32 encode/decode helpers
// ---------------------------------------------------------------------------
std::string base32Encode(const std::vector<BYTE>& data)
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

bool generateRandomBytes(std::vector<BYTE>& out, ULONG len)
{
    out.resize(len);
    BCRYPT_ALG_HANDLE hAlg = NULL;
    if (!BCRYPT_SUCCESS(BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_RNG_ALGORITHM, NULL, 0)))
        return false;
    NTSTATUS status = BCryptGenRandom(hAlg, out.data(), len, 0);
    BCryptCloseAlgorithmProvider(hAlg, 0);
    return BCRYPT_SUCCESS(status);
}

// ---------------------------------------------------------------------------
// Simple URL percent-encoding for the otpauth URI
// ---------------------------------------------------------------------------
std::string urlEncode(const char* s)
{
    std::string out;
    for (const unsigned char* p = reinterpret_cast<const unsigned char*>(s); *p; ++p)
    {
        unsigned char c = *p;
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') ||
            c == '-' || c == '_' || c == '.' || c == '~')
        {
            out.push_back(static_cast<char>(c));
        }
        else
        {
            char buf[4];
            sprintf_s(buf, "%%%02X", c);
            out += buf;
        }
    }
    return out;
}

// ---------------------------------------------------------------------------
// INI path: same directory as the matching TokenValidator DLL so the DLL's
// fallback can find it. If the DLL is not present, fall back to the EXE name.
// ---------------------------------------------------------------------------
bool getIniPath(char* path, size_t size)
{
    if (!GetModuleFileNameA(NULL, path, static_cast<DWORD>(size)))
        return false;

    // Look for TokenValidator.dll (x64) or TokenValidator32.dll (x86) next to us.
    char* lastSlash = strrchr(path, '\\');
    char baseName[MAX_PATH] = {};
    if (lastSlash)
        strncpy_s(baseName, sizeof(baseName), path, lastSlash - path + 1); // includes backslash
    else
        baseName[0] = '\0';

    char candidate[MAX_PATH] = {};
    for (const char* dllName : { "TokenValidator.dll", "TokenValidator32.dll" })
    {
        sprintf_s(candidate, sizeof(candidate), "%s%s", baseName, dllName);
        if (GetFileAttributesA(candidate) != INVALID_FILE_ATTRIBUTES)
        {
            strncpy_s(path, size, candidate, _TRUNCATE);
            break;
        }
    }

    char* dot = strrchr(path, '.');
    if (!dot)
        return false;
    strcpy_s(dot, size - (dot - path), ".totp.ini");
    return true;
}

// ---------------------------------------------------------------------------
// Store secret via DPAPI (LocalMachine scope) in the INI file
// ---------------------------------------------------------------------------
bool writeEncryptedIni(const char* user, const std::string& secretUtf8)
{
    char iniPath[MAX_PATH] = {};
    if (!getIniPath(iniPath, MAX_PATH))
        return false;

    DATA_BLOB in = { static_cast<DWORD>(secretUtf8.size()),
                     const_cast<BYTE*>(reinterpret_cast<const BYTE*>(secretUtf8.data())) };
    DATA_BLOB out = {};

    if (!CryptProtectData(&in, L"UltraVNC TOTP secret", NULL, NULL, NULL,
                          CRYPTPROTECT_LOCAL_MACHINE, &out))
    {
        return false;
    }

    char* base64 = NULL;
    DWORD base64Len = 0;
    if (!CryptBinaryToStringA(out.pbData, out.cbData, CRYPT_STRING_BASE64, NULL, &base64Len))
    {
        LocalFree(out.pbData);
        return false;
    }
    base64 = static_cast<char*>(malloc(base64Len));
    if (!base64)
    {
        LocalFree(out.pbData);
        return false;
    }
    if (!CryptBinaryToStringA(out.pbData, out.cbData, CRYPT_STRING_BASE64, base64, &base64Len))
    {
        free(base64);
        LocalFree(out.pbData);
        return false;
    }
    LocalFree(out.pbData);

    BOOL ok = WritePrivateProfileStringA("TOTP", user, base64, iniPath);
    free(base64);
    return ok != FALSE;
}

// ---------------------------------------------------------------------------
// Store secret in Credential Manager for the interactive/service account
// ---------------------------------------------------------------------------
bool writeCredential(const char* user, const std::string& secretUtf8)
{
    wchar_t targetName[256] = {};
    wchar_t wideUser[256] = {};
    if (MultiByteToWideChar(CP_UTF8, 0, user, -1, wideUser, 256) <= 0)
        return false;
    swprintf_s(targetName, L"UltraVNC_TOTP_%s", wideUser);

    CREDENTIALW cred = {};
    cred.Type = CRED_TYPE_GENERIC;
    cred.TargetName = targetName;
    cred.CredentialBlobSize = static_cast<DWORD>(secretUtf8.size());
    cred.CredentialBlob = const_cast<BYTE*>(reinterpret_cast<const BYTE*>(secretUtf8.data()));
    cred.Persist = CRED_PERSIST_LOCAL_MACHINE;
    cred.UserName = wideUser;

    return CredWriteW(&cred, 0) == TRUE;
}

// ---------------------------------------------------------------------------
// Write a 24-bit monochrome BMP QR code image
// ---------------------------------------------------------------------------
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

bool writeQrBmp(const char* text, const char* filename, int moduleScale = 8, int quietModules = 4)
{
    try
    {
        qrcodegen::QrCode qr = qrcodegen::QrCode::encodeText(text, qrcodegen::QrCode::Ecc::MEDIUM);
        int modules = qr.getSize();
        int imgSize = modules + 2 * quietModules;
        int width = imgSize * moduleScale;
        int height = width;
        int rowSize = ((width * 3 + 3) & ~3);
        int pixelArraySize = rowSize * height;

        BMPFILEHEADER fh = {};
        BMPINFOHEADER ih = {};
        fh.bfType = 0x4D42; // 'BM'
        fh.bfSize = sizeof(fh) + sizeof(ih) + pixelArraySize;
        fh.bfOffBits = sizeof(fh) + sizeof(ih);
        ih.biSize = sizeof(ih);
        ih.biWidth = width;
        ih.biHeight = height;
        ih.biPlanes = 1;
        ih.biBitCount = 24;
        ih.biCompression = BI_RGB;
        ih.biSizeImage = pixelArraySize;

        std::vector<BYTE> pixels(pixelArraySize, 0xFF); // white background
        auto setPixel = [&](int x, int y, BYTE r, BYTE g, BYTE b)
        {
            // BMP rows are bottom-up
            int row = (height - 1 - y);
            BYTE* p = pixels.data() + row * rowSize + x * 3;
            p[0] = b; p[1] = g; p[2] = r;
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
                            setPixel(startX + dx, startY + dy, 0, 0, 0);
                }
            }
        }

        HANDLE hFile = CreateFileA(filename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                                   FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile == INVALID_HANDLE_VALUE)
            return false;
        DWORD written = 0;
        WriteFile(hFile, &fh, sizeof(fh), &written, NULL);
        WriteFile(hFile, &ih, sizeof(ih), &written, NULL);
        WriteFile(hFile, pixels.data(), pixelArraySize, &written, NULL);
        CloseHandle(hFile);
        return true;
    }
    catch (...)
    {
        return false;
    }
}

} // namespace

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s <username> [base32-secret]\n", argv[0]);
        fprintf(stderr, "If the base32 secret is omitted, a random 160-bit secret is generated.\n");
        return 1;
    }

    const char* user = argv[1];
    std::string secretBase32;

    if (argc >= 3)
    {
        secretBase32 = argv[2];
        // Basic sanitization: uppercase, remove spaces/hyphens.
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
        std::vector<BYTE> randBytes;
        if (!generateRandomBytes(randBytes, 20))
        {
            fprintf(stderr, "Failed to generate random secret\n");
            return 1;
        }
        secretBase32 = base32Encode(randBytes);
    }

    // Make sure the secret is valid base32 length/content (multiples of 8 chars).
    if (secretBase32.empty() || (secretBase32.size() % 8) != 0)
    {
        fprintf(stderr, "Invalid base32 secret (length must be a multiple of 8)\n");
        return 1;
    }

    if (!writeCredential(user, secretBase32))
    {
        fprintf(stderr, "Warning: failed to write Credential Manager entry (code %lu)\n", GetLastError());
    }

    if (!writeEncryptedIni(user, secretBase32))
    {
        fprintf(stderr, "Warning: failed to write encrypted INI fallback (code %lu)\n", GetLastError());
    }

    char iniPath[MAX_PATH] = {};
    getIniPath(iniPath, MAX_PATH);

    std::string uri = "otpauth://totp/UltraVNC:";
    uri += urlEncode(user);
    uri += "?secret=";
    uri += secretBase32;
    uri += "&issuer=UltraVNC";

    char bmpPath[MAX_PATH] = {};
    if (GetModuleFileNameA(NULL, bmpPath, MAX_PATH))
    {
        char* dot = strrchr(bmpPath, '.');
        if (dot)
        {
            char suffix[256];
            sprintf_s(suffix, "_%s_totp.bmp", user);
            strcpy_s(dot, MAX_PATH - (dot - bmpPath), suffix);
        }
    }
    if (!bmpPath[0])
        sprintf_s(bmpPath, "%s_totp.bmp", user);

    if (!writeQrBmp(uri.c_str(), bmpPath))
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
    printf("INI fallback: %s\n", iniPath);
    return 0;
}
