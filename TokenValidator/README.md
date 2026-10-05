# TokenValidator sample DLL

This is a sample validation DLL for the new UltraVNC **VeNCrypt token-TLS** authentication mode.

It supports two credential formats:

* **Static token** – a 64-character token, kept as a compatibility/test fallback.
* **TOTP code** – a 6- to 8-digit time-based one-time password from any authenticator app that implements RFC 6238, such as **Microsoft Authenticator**, Google Authenticator, or Authy.

## Expected DLL export

```cpp
extern "C" __declspec(dllexport) int __cdecl validate_token(const char* user, const char* token);
```

Return `1` to allow the connection, `0` to reject it.

## Static-token fallback

For testing and backwards compatibility the original check is still supported:

* username: `test`
* token: exactly 64 ASCII characters, all `'a'`

```
aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa
```

## TOTP mode

When the supplied credential is **6, 7, or 8 digits only**, the DLL validates it as a RFC 6238 TOTP code:

* SHA1
* 30-second time step
* 6-digit output
* comparison window of the previous, current and next time step (tolerance for clock skew)

The per-user base32 secret is read from one of two places:

1. **Windows Credential Manager** – target name `UltraVNC_TOTP_<username>`.
2. **DPAPI-encrypted INI fallback** – `<TokenValidator DLL>.totp.ini`. This is useful when WinVNC runs as `SYSTEM`, because Credential Manager entries written by an interactive admin are not visible to the service.

The INI is encrypted with `CryptProtectData` (`CRYPTPROTECT_LOCAL_MACHINE`) and the secret is stored as a base64-encoded blob. Restrict the file ACL to `SYSTEM` and `Administrators` in production.

## Building

Open a Visual Studio 2019 (or newer) developer command prompt and run:

```cmd
build.bat        :: x64 DLL and TokenSetup.exe
build32.bat      :: x86 DLL only
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

Open the BMP file and scan it with Microsoft Authenticator, or enter the base32 secret manually. The secret is also written to Windows Credential Manager and the encrypted INI, so the server can validate the same codes.

## Configure WinVNC

Add these keys to the WinVNC INI file (usually `C:\ProgramData\UltraVNC\ultravnc.ini` or the service INI):

```ini
[admin]
EnableVeNCryptTLS=1
VeNCryptTokenDLLPath=C:\path\to\TokenValidator.dll
```

If `VeNCryptTLSCertThumbprint` is omitted, WinVNC will generate/use a self-signed certificate named `CN=UltraVNC-WebViewer` and persist its thumbprint automatically.

## Connect from UltraVNC Viewer

Use the existing `-user` and `-token` switches. With the TOTP changes, `-token` can be either the 64-character static token or the current 6-digit TOTP code from the authenticator app:

```cmd
vncviewer.exe 127.0.0.1 -user test -token 123456
```

If `-token` is omitted, the viewer will prompt for username + token when the server advertises `secTypeTokenTLS`.

## Connect from TigerVNC / TurboVNC / other viewers

Any viewer that supports standard VeNCrypt `TLSPlain` or `X509Plain` can connect:

1. Choose **Encryption: TLS with X509 certificates** (or **TLS anonymous** if the server advertises it).
2. Choose **Authentication: Username and password**.
3. Accept the self-signed certificate.
4. Enter `test` as the username and the current 6-digit TOTP code as the password.

This mirrors how TurboVNC exposes TOTP via the OATH Toolkit (`pam_oath` / `users.oath`).

## Important notes

* The server clock must be reasonably accurate. The DLL allows a one-step window, but large drift will cause valid codes to be rejected.
* When WinVNC runs as a service under `LOCAL SYSTEM`, run `TokenSetup.exe` with that identity so the Credential Manager entry lands in the correct vault. The easiest way is to use `PsExec -s`:

  ```cmd
  PsExec -s -i TokenSetup.exe test
  ```

  If that is inconvenient, the encrypted INI fallback (`TokenValidator.totp.ini`) is read by the DLL as well.
* The 64-character static token mode remains single-factor. For true two-factor authentication, combine TOTP with a separate password or rely on the Windows logon layer.
