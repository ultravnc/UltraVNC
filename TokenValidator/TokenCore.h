// TokenCore.h - shared helpers for UltraVNC TokenValidator
//
// TOTP secret management, QR rendering, and URI building used by both
// TokenValidator.dll (validation + management UI) and TokenSetup.exe (CLI).

#pragma once

#include <windows.h>
#include <string>
#include <vector>

namespace TokenCore {

// ---------------------------------------------------------------------------
// Secret generation and base32 helpers
// ---------------------------------------------------------------------------
bool GenerateRandomBase32Secret(std::string& out, size_t bits = 160);
bool ValidateBase32(const std::string& s);
bool Base32Decode(const std::string& in, std::vector<BYTE>& out);

// ---------------------------------------------------------------------------
// Secret storage - Credential Manager (preferred) + DPAPI-encrypted INI
// ---------------------------------------------------------------------------
bool SetSecretStorePathsFromDllPath(const char* dllPath);
bool SetSecretStorePathsFromModuleHandle(HMODULE hModule);
std::string GetSecretStoreIniPath();

bool ReadTotpSecret(const std::string& user, std::string& secretOut);
bool WriteTotpSecret(const std::string& user, const std::string& secret);
bool DeleteTotpSecret(const std::string& user);
std::vector<std::string> ListTotpUsers();

// ---------------------------------------------------------------------------
// TOTP URI and QR output
// ---------------------------------------------------------------------------
std::string BuildOtpAuthUri(const std::string& user,
                            const std::string& secret,
                            const std::string& issuer = "UltraVNC");

// Write a 24-bit monochrome BMP to a file.
bool WriteQrBmpToFile(const std::string& text,
                       const std::string& filename,
                       int moduleScale = 6,
                       int quietModules = 4);

// Render a QR code as an in-memory HBITMAP suitable for a SS_BITMAP control.
// The caller owns the returned bitmap and must call DeleteObject().
HBITMAP RenderQrBitmap(HWND hwndReference,
                        const std::string& text,
                        int moduleScale = 6,
                        int quietModules = 4);

} // namespace TokenCore
