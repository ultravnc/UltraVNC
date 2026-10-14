// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: MIT
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
//

// TokenValidatorPlugin.h - public C ABI for UltraVNC TokenValidator DLLs.
//
// A validator DLL loaded by WinVNC must export at least validate_token().
// It may optionally export issue_session_token() to support versioned
// subtype-300 session-token reconnect, and tv_has_management_ui() / tv_show_management_ui()
// to provide an in-product user/token management dialog.
//
// Build for the same architecture as the WinVNC executable that loads the DLL
// (x64 or x86). All functions use the __cdecl calling convention and are
// resolved by name via LoadLibrary/GetProcAddress.

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

// Feature capability bits reported by tv_get_capabilities().
// Credential format is entirely the DLL's business; these flags only describe
// which protocol features the DLL supports.
#define TV_CAP_SESSION_TOKEN  0x01  // exports issue_session_token / validates session tokens
#define TV_CAP_MANAGEMENT_UI  0x02  // exports tv_show_management_ui
#define TV_CAP_SASL           0x04  // exports sasl_lookup (SCRAM verifier store)

// Returns the TV_CAP_* feature bitmask supported by this DLL.
// Optional: if the export is missing, clients may probe individual
// exports (issue_session_token, tv_has_management_ui) instead.
typedef int (__cdecl *TV_GET_CAPABILITIES)(void);

// Issue a server-side session token for the given user after a successful
// primary authentication. The returned token is valid for a server-defined
// lifetime and can be used with validate_token() instead of a TOTP code.
typedef int (__cdecl *TV_ISSUE_SESSION_TOKEN)(const char* user, char* outBuf, size_t outBufSize);

// Return the stored SCRAM-SHA-256 verifier for the given user as a
// null-terminated PostgreSQL-style string:
//   "<iterations>:<saltB64>:<storedKeyB64>:<serverKeyB64>"
// Used by WinVNC to run the SASL server exchange (VeNCrypt 263/264) without
// ever seeing the user's plaintext password. Returns 1 on success, 0 when
// the user has no SCRAM credential.
typedef int (__cdecl *TV_SASL_LOOKUP)(const char* user, char* outBuf, size_t outBufSize);

// Required export (must exist or authentication always fails):
//   validate_token
//
// Optional exports:
//   tv_get_capabilities      - reports TV_CAP_* feature flags.
//   issue_session_token      - needed for subtype-300 session-token reconnect.
//   sasl_lookup              - needed for subtype-263/264 SASL (SCRAM) auth.
//   tv_has_management_ui     - advertises that tv_show_management_ui exists.
//   tv_show_management_ui    - opens the validator-specific management dialog.
//   tv_get_last_error        - optional diagnostic message buffer.
//
// Clients should use LoadLibrary/GetProcAddress with the typedefs above;
// linking against an import library is not required.

#ifdef __cplusplus
}
#endif

#endif // TOKEN_VALIDATOR_PLUGIN_H
