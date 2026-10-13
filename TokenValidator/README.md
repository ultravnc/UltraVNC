# TokenValidator sample DLL

This is a validation DLL for the new UltraVNC **VeNCrypt token-TLS** authentication mode.

It supports two credential formats:

* **TOTP code** – a 6- to 8-digit time-based one-time password from any authenticator app that implements RFC 6238, such as **Microsoft Authenticator**, Google Authenticator, or Authy.
* **Session token** – a 64-character server-issued token used by versioned subtype 300 for reconnects without prompting for a new TOTP code.

## Expected DLL exports

```cpp
extern "C" __declspec(dllexport) int __cdecl validate_token(const char* user, const char* token);
extern "C" __declspec(dllexport) int __cdecl tv_get_capabilities(void);
extern "C" __declspec(dllexport) int __cdecl issue_session_token(const char* user, char* outBuf, size_t outBufSize);
extern "C" __declspec(dllexport) int __cdecl tv_has_management_ui(void);
extern "C" __declspec(dllexport) int __cdecl tv_show_management_ui(HWND hwndParent);
extern "C" __declspec(dllexport) int __cdecl tv_get_last_error(char* buf, size_t bufSize);
```

* `validate_token` returns `1` to allow the connection, `0` to reject it.
* `tv_get_capabilities` returns the `TV_CAP_*` feature bitmask (see below).
* `issue_session_token` generates a server-side session token after a successful TOTP auth.
* `tv_has_management_ui` / `tv_show_management_ui` let WinVNC display a dialog for user management.
* `tv_get_last_error` is an optional diagnostic hook.

## Capability negotiation

The credential itself is opaque to WinVNC — the DLL receives `user + token` and
decides what a valid token means (TOTP code, a customer-specific 64-byte token,
a database lookup — anything). What the server *does* need to know is which
optional protocol features the DLL implements. `tv_get_capabilities` reports
that as a bitmask:

```cpp
#define TV_CAP_SESSION_TOKEN  0x01  // exports issue_session_token and
                                    // validate_token accepts the tokens it issues
#define TV_CAP_MANAGEMENT_UI  0x02  // exports tv_show_management_ui
```

WinVNC uses the flags as follows:

* **`TV_CAP_SESSION_TOKEN`** — the validator can issue and accept reconnect
  session tokens. The server always offers versioned subtype **300**; clients
  request session-token issuance through its encrypted feature flags. Without
  this capability the server returns an empty session-token response.
* **`TV_CAP_MANAGEMENT_UI`** — the WinVNC **Manage users...** button on the
  SSL/TLS tab is enabled only when this flag is set.

If a DLL does **not** export `tv_get_capabilities`, WinVNC falls back to
probing the individual exports: `issue_session_token` implies
`TV_CAP_SESSION_TOKEN`, and `tv_has_management_ui`/`tv_show_management_ui`
imply `TV_CAP_MANAGEMENT_UI`. Keep the flags consistent with your exports.

The DLL is loaded dynamically with `LoadLibrary`/`GetProcAddress` from the path
in `VeNCryptTokenDLLPath` and called with the C ABI (`extern "C"`, `__cdecl`).
Session-token records use the same machine-protected INI so they survive a
service desktop-process restart. All
RFB traffic after authentication — including the UltraVNC file transfer —
stays inside the TLS channel, so tokens never cross the wire unencrypted.

## TOTP mode

When the supplied credential is **6, 7, or 8 digits only**, the DLL validates it as a RFC 6238 TOTP code:

* SHA1
* 30-second time step
* 6-digit output
* comparison window of the previous, current and next time step (tolerance for clock skew)

The per-user base32 secret is read from one of two places:

1. **Windows Credential Manager** – target name `UltraVNC_TOTP_<username>`.
2. **DPAPI-encrypted INI fallback** – `<TokenValidator DLL>.totp.ini`. This is useful when WinVNC runs as `SYSTEM`, because Credential Manager entries written by an interactive admin are not visible to the service.

The INI is encrypted with `CryptProtectData` (`CRYPTPROTECT_LOCAL_MACHINE`) and the secret is stored as a base64-encoded blob. The DLL automatically restricts the file ACL to `SYSTEM`, `BUILTIN\Administrators`, and the current user.

## Session-token reconnect (versioned subtype 300)

After a successful TOTP authentication, the server calls `issue_session_token` to create a random 64-character token. The token is sent to the viewer inside the TLS tunnel.

* The viewer stores it in memory and uses it on reconnects instead of a new TOTP code.
* The token remains pending until it is used or replaced; it does not expire while the desktop connection remains active.
* Only **one pending session token** is kept per user. Every successful reconnect immediately issues a replacement and revokes the token just used.
* A fresh TOTP login also issues a replacement, revoking any abandoned token.
* The server record is protected with machine-scope DPAPI and survives a WinVNC desktop-process restart.
* The viewer token remains memory-only; closing the viewer still requires a fresh TOTP.
* If the token is rejected, the viewer clears it and prompts for a fresh TOTP.

## Building

Open a Visual Studio 2019 (or newer) developer command prompt and run:

```cmd
build.bat        :: x64 TokenValidator.dll and TokenSetup.exe
build32.bat      :: x86 TokenValidator32.dll only
```

## Registering a user

Use `TokenSetup.exe` to generate or import a base32 secret for a user. The tool stores the secret in Credential Manager **and** in the encrypted INI fallback, and writes a scannable QR-code BMP.

Generate a random secret:

```cmd
TokenSetup.exe test
```

Import an existing base32 secret:

```cmd
TokenSetup.exe test JBSWY3DPEHPK3PXP
```

Output example:

```
User: test
Base32 secret: K5AZCCL5WAI5ABRALWHGATHGJJJPJK...
Provisioning URI: otpauth://totp/UltraVNC:test?secret=K5AZCCL5WAI5ABRALWHGATHGJJJPJK...&issuer=UltraVNC
QR code image: TokenSetup_test_totp.bmp
Credential Manager target: UltraVNC_TOTP_test
INI fallback: TokenValidator.totp.ini
```

Open the BMP file and scan it with Microsoft Authenticator, or enter the base32 secret manually.

## Manage users from WinVNC

The WinVNC **Admin Properties → SSL/TLS** tab has a **Manage users...** button. It loads the configured `VeNCryptTokenDLLPath` and calls `tv_show_management_ui`. The built-in TokenValidator shows a dialog with:

* Add user (generates secret + visible QR code)
* Remove user
* Reset secret (regenerates + new QR)
* Show QR again

Alternate validator DLLs can implement `tv_has_management_ui` / `tv_show_management_ui` to provide their own user-management UI/options.

## Writing your own validator DLL

You can replace `TokenValidator.dll` with any DLL that exports the C ABI in `TokenValidatorPlugin.h`:

| Export | Required | Purpose |
|---|---|---|
| `validate_token(user, token)` | **yes** | Accept/reject a credential. Return `1`/`0`. Enough for subtype 300. |
| `tv_get_capabilities` | no | Returns the `TV_CAP_*` feature bitmask. If omitted, WinVNC probes the optional exports instead. |
| `issue_session_token(user, outBuf, outBufSize)` | no | Issue a subtype-300 reconnect token. Advertise via `TV_CAP_SESSION_TOKEN`. |
| `tv_has_management_ui` | no | Advertises whether a management dialog is available (legacy probe). |
| `tv_show_management_ui` | no | Opens your validator-specific management dialog. Advertise via `TV_CAP_MANAGEMENT_UI`. |
| `tv_get_last_error` | no | Diagnostic message buffer. |

The DLL must be built for the same architecture as the `winvnc.exe` that loads it (x64 or x86) and the export names must match exactly with `__cdecl`.

A minimal working skeleton is provided as `CustomValidatorSkeleton.cpp`. It shows a simple hard-coded validator, a session-token stub, and a placeholder management dialog. Copy it, rename it, and build it with:

```cmd
cl.exe -LD -FeMyTokenValidator.dll CustomValidatorSkeleton.cpp advapi32.lib bcrypt.lib crypt32.lib user32.lib gdi32.lib /EHsc
```

Then point `VeNCryptTokenDLLPath` to your DLL.

## Configure WinVNC

Add these keys to the WinVNC INI file (usually `C:\ProgramData\UltraVNC\ultravnc.ini` or the service INI):

```ini
[admin]
EnableVeNCryptTLS=1
RequireVeNCryptTLS=0
VeNCryptTokenDLLPath=C:\path\to\TokenValidator.dll
```

If `VeNCryptTLSCertThumbprint` is omitted, WinVNC will generate/use a self-signed certificate and persist its thumbprint automatically.

## Connect from UltraVNC Viewer

Use the existing `-user` and `-token` switches, or check **Use TLS (VeNCrypt)** in the Security tab. With the TOTP changes, `-token` is the current 6-digit TOTP code from the authenticator app:

```cmd
vncviewer.exe 127.0.0.1 -user test -token 123456
```

If `-token` is omitted, the viewer prompts for username + token when the server advertises `secTypeTokenTLS` (300). On reconnect after a desktop switch, the viewer sends method 2 with the stored session token automatically.

## Connect from TigerVNC / TurboVNC / other viewers

Any viewer that supports standard VeNCrypt `TLSPlain` or `X509Plain` can connect:

1. Choose **Encryption: TLS with X509 certificates** (or **TLS anonymous** if the server advertises it).
2. Choose **Authentication: Username and password**.
3. Accept the self-signed certificate.
4. Enter the username and the current 6-digit TOTP code as the password.

This mirrors how TurboVNC exposes TOTP via the OATH Toolkit (`pam_oath` / `users.oath`).

## Important notes

* The server clock must be reasonably accurate. The DLL allows a one-step window, but large drift will cause valid codes to be rejected.
* When WinVNC runs as a service under `LOCAL SYSTEM`, run `TokenSetup.exe` with that identity so the Credential Manager entry lands in the correct vault. The easiest way is to use `PsExec -s`:

  ```cmd
  PsExec -s -i TokenSetup.exe test
  ```

  If that is inconvenient, the encrypted INI fallback (`TokenValidator.totp.ini`) is read by the DLL as well.
* Session tokens are only valid while WinVNC keeps the validator DLL loaded. They are not persisted across server restarts.
* For true two-factor authentication, combine TOTP with a separate password or rely on the Windows logon layer.
