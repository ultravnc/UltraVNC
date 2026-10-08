// TokenValidatorPlugin.h - public C ABI for UltraVNC TokenValidator DLLs.
//
// A validator DLL loaded by WinVNC must export at least validate_token().
// Optionally it may export tv_has_management_ui() and tv_show_management_ui()
// to provide an in-product user/token management dialog.

#ifndef TOKEN_VALIDATOR_PLUGIN_H
#define TOKEN_VALIDATOR_PLUGIN_H

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

// Validate a username + token/credential.
// Returns 1 on success, 0 on failure.
typedef int (__cdecl *TV_VALIDATE_TOKEN)(const char* user, const char* token);

// Returns 1 if this DLL provides a management UI, 0 otherwise.
typedef int (__cdecl *TV_HAS_MANAGEMENT_UI)(void);

// Show the management dialog as a child of hwndParent.
// Returns 1 if the user OK'd / closed normally, 0 on error or cancel.
typedef int (__cdecl *TV_SHOW_MANAGEMENT_UI)(HWND hwndParent);

// Optional diagnostic: copies a null-terminated error string into buf.
// Returns the number of bytes written (excluding terminator), or 0 if none.
typedef int (__cdecl *TV_GET_LAST_ERROR)(char* buf, size_t bufSize);

// Issue a server-side session token for the given user after a successful
// primary authentication. The returned token is valid for a server-defined
// lifetime and can be used with validate_token() instead of a TOTP code.
typedef int (__cdecl *TV_ISSUE_SESSION_TOKEN)(const char* user, char* outBuf, size_t outBufSize);

// Expected DLL export names (the implementation must use these names and the
// __cdecl convention):
//   validate_token
//   issue_session_token
//   tv_has_management_ui
//   tv_show_management_ui
//   tv_get_last_error
//
// Clients should use LoadLibrary/GetProcAddress with the typedefs above;
// linking against an import library is not required.

#ifdef __cplusplus
}
#endif

#endif // TOKEN_VALIDATOR_PLUGIN_H
