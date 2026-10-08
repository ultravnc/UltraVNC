// CustomValidatorSkeleton.cpp
//
// Minimal example of a replacement TokenValidator.dll for UltraVNC.
// Copy this file, rename it, and build with:
//
//   cl.exe -LD -FeMyTokenValidator.dll CustomValidatorSkeleton.cpp advapi32.lib bcrypt.lib crypt32.lib user32.lib gdi32.lib /EHsc
//
// The exported names must match exactly and use __cdecl.
// TokenValidatorPlugin.h contains the same function signatures.

#include <windows.h>
#include <bcrypt.h>
#include <stdio.h>
#include <string.h>

#pragma comment(lib, "bcrypt.lib")

// In-memory store for the last issued session token. A real DLL would keep a
// per-user table with expiries; this skeleton accepts one token at a time.
static char g_issuedToken[65] = {};

// ---------------------------------------------------------------------------
// Required export
// ---------------------------------------------------------------------------
// Return 1 to allow the connection, 0 to reject it.
// This example accepts user "demo" with a 6-digit TOTP-like code "123456",
// or the session token previously returned by issue_session_token().
extern "C" __declspec(dllexport) int __cdecl
validate_token(const char* user, const char* token)
{
    if (!user || !token)
        return 0;

    if (strcmp(user, "demo") == 0 && strcmp(token, "123456") == 0)
        return 1;

    if (strcmp(user, "demo") == 0 && g_issuedToken[0] &&
        strcmp(token, g_issuedToken) == 0)
        return 1;

    return 0;
}

// ---------------------------------------------------------------------------
// Optional export for TokenTLSv2 (subtype 301)
// ---------------------------------------------------------------------------
// Called after a successful validate_token() to issue a session token for
// reconnects. The server sends this token back to the viewer.
// If you do not need session-token reconnect, do not export this function;
// WinVNC will simply not use TokenTLSv2 and fall back to TokenTLS.
extern "C" __declspec(dllexport) int __cdecl
issue_session_token(const char* user, char* outBuf, size_t outBufSize)
{
    if (!user || !outBuf || outBufSize < 65)
        return 0;

    // Cryptographically random 64-hex-char session token.
    UCHAR rnd[32];
    if (BCryptGenRandom(NULL, rnd, sizeof(rnd),
                        BCRYPT_USE_SYSTEM_PREFERRED_RNG) != 0)
        return 0;

    static const char hex[] = "0123456789abcdef";
    for (int i = 0; i < 32; ++i) {
        outBuf[2 * i]     = hex[rnd[i] >> 4];
        outBuf[2 * i + 1] = hex[rnd[i] & 0x0f];
    }
    outBuf[64] = '\0';

    // Remember it so validate_token() can accept it on reconnect.
    strcpy_s(g_issuedToken, sizeof(g_issuedToken), outBuf);
    return 1;
}

// ---------------------------------------------------------------------------
// Optional capabilities export
// ---------------------------------------------------------------------------
// Report which optional features this DLL supports. Flags are defined in
// TokenValidatorPlugin.h. Keep these consistent with your exports:
//   TV_CAP_SESSION_TOKEN  - you export issue_session_token AND validate_token
//                           accepts the tokens it issues.
//   TV_CAP_MANAGEMENT_UI  - you export tv_show_management_ui.
// If this export is missing entirely, WinVNC falls back to probing the
// individual exports above.
#define TV_CAP_SESSION_TOKEN  0x01
#define TV_CAP_MANAGEMENT_UI  0x02

extern "C" __declspec(dllexport) int __cdecl
tv_get_capabilities(void)
{
    return TV_CAP_SESSION_TOKEN | TV_CAP_MANAGEMENT_UI;
}

// ---------------------------------------------------------------------------
// Optional management UI exports
// ---------------------------------------------------------------------------
// Return 1 if you want WinVNC to show a "Manage users..." button and let it
// open your custom dialog. Return 0 to disable the button.
extern "C" __declspec(dllexport) int __cdecl
tv_has_management_ui(void)
{
    return 1;
}

// Show your management dialog. hwndParent is the WinVNC properties window.
// You can use DialogBoxParam, TaskDialog, or any custom UI. Return 1 on success.
extern "C" __declspec(dllexport) int __cdecl
tv_show_management_ui(HWND hwndParent)
{
    MessageBoxA(hwndParent,
                "Replace this with your own user/token management dialog.",
                "Custom TokenValidator",
                MB_OK | MB_ICONINFORMATION);
    return 1;
}

// ---------------------------------------------------------------------------
// Optional diagnostics
// ---------------------------------------------------------------------------
extern "C" __declspec(dllexport) int __cdecl
tv_get_last_error(char* buf, size_t bufSize)
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
    (void)hModule;
    (void)lpReserved;
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hModule);
        break;
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
