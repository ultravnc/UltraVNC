# UltraVNC VeNCrypt Token Authentication — Wire Protocol

Specification of the TLS + token authentication used between UltraVNC Viewer
and WinVNC. Intended for anyone writing a compatible client or remote app.

All integers are big-endian (network byte order) unless stated otherwise.

## 1. Overview

The protocol stacks standard RFB 3.8 on top of a negotiated TLS channel.
Authentication is delegated to a replaceable validator DLL on the server —
UltraVNC treats the token as an opaque string and never inspects its content.

```
TCP
 └─ RFB 3.8 version + security-type negotiation
     └─ VeNCrypt version + subtype negotiation      (plaintext)
         └─ TLS handshake                           (TLS 1.2+)
             └─ authentication exchange             (inside TLS)
                 └─ SecurityResult + session token  (inside TLS)
                     └─ normal RFB session          (inside TLS)
                          incl. file transfer
```

## 2. RFB negotiation

Standard RFB 3.8:

```
S->C  "RFB 003.008\n"                    (12 bytes)
C->S  "RFB 003.008\n"                    (12 bytes)
S->C  u8 count + count * u8              (security-type list)
C->S  u8 = 19                            (select rfbVeNCrypt)
```

`rfbVeNCrypt` (19) is offered only when `EnableVeNCryptTLS` is set. If
`RequireVeNCryptTLS` is set it is the *only* type offered.

## 3. VeNCrypt negotiation

```
S->C  u8 major, u8 minor                 server version = 0.2  (bytes 00 02)
C->S  u8 major, u8 minor                 client version (must be >= 0.2)
S->C  u8 status                          0x00 = OK, 0xFF = unsupported
S->C  u8 count + count * u32             offered subtypes
C->S  u32                                chosen subtype
S->C  u8 0x01                            subtype acknowledged (Ack)
```

The `0x01` Ack is required by the current VeNCrypt spec (rfbproto section
7.2.4: *"For TLS and X509 subtypes, the server then sends a one byte
response"*). It applies only to TLS/X509 subtypes; since this server offers
no plaintext subtypes, it is always sent.

**Compatibility caveat:** the original `vencrypt.txt` draft lacked this byte
and some implementations still follow it. TigerVNC/KasmVNC neither send nor
read the Ack — TigerVNC viewers cannot connect to this server (the extra
byte corrupts their TLS stream), and the UltraVNC viewer cannot connect to
TigerVNC VeNCrypt servers (it waits for an Ack that never arrives). QEMU
and libvncclient-based clients (Remmina, bVNC, Vinagre, …) read the Ack per
spec and interoperate correctly.

Then the **TLS handshake** runs on the same socket. The server presents a
certificate from its configured Windows certificate store (a self-signed
certificate is generated on first use if none is configured). No client
certificate is required. Both TLS 1.2 and 1.3 work; the negotiated version is
whatever both sides support.

## 4. Subtypes

| Value | Name              | Inner authentication                          |
|-------|-------------------|-----------------------------------------------|
| 257   | secTypeTLSNone    | none — result OK immediately                  |
| 261   | secTypeX509Vnc    | classic VNC DES challenge/response            |
| 259   | secTypeTLSPlain   | user + pass message (5), validated by DLL     |
| 262   | secTypeX509Plain  | user + pass message (5), validated by DLL     |
| 300   | secTypeTokenTLS   | user + token message (5), validated by DLL    |
| 301   | secTypeTokenTLSv2 | same + session token issued on success (6)    |

Subtype 301 is offered **only** when the configured validator DLL reports the
`TV_CAP_SESSION_TOKEN` capability (section 7). A server whose validator lacks
that capability advertises 300 but never 301.

The `TLS*`/`X509*` pairs behave identically in this implementation — the
certificate is always presented and the client is expected to verify it (the
UltraVNC viewer shows a warning dialog with certificate details and thumbprint
persistence on verification failure).

## 5. Credential message (subtypes 259, 262, 300, 301)

Sent inside the TLS channel, immediately after the handshake. Uses the
standard VeNCrypt "Plain" layout so generic clients can reuse their existing
TLSPlain code path:

```
C->S  u32 userLen      1..256 bytes
C->S  u32 tokenLen     6..64 bytes
C->S  userLen bytes    username (UTF-8/ASCII)
C->S  tokenLen bytes   token (ASCII, opaque to the server)
```

Out-of-range lengths close the connection. The server passes both strings
verbatim to the validator DLL's `validate_token()`; semantics (TOTP, session
token, API key, …) belong to the DLL.

## 6. SecurityResult and session token

Standard RFB SecurityResult, inside the TLS channel:

```
S->C  u32 result       0 = OK, 1 = failed
      if failed:
S->C  u32 reasonLen + reasonLen bytes   UTF-8 reason string
```

On `result = 0` the client proceeds to `ClientInit` and the normal RFB session
— framebuffer, input, file transfer — all inside TLS.

**Subtype 301 only:** immediately after `SecurityResult = 0` the server sends:

```
S->C  u8 tokenLen + tokenLen bytes      session token
```

`tokenLen` may be 0 (no token issued — e.g. DLL missing `issue_session_token`);
the client must still read the length byte. The token is at most 64 bytes,
in-memory only, and meant to be replayed as the `token` field on a subsequent
connection instead of prompting the user again (e.g. after a desktop switch or
auto-reconnect). Lifetime and acceptance policy are defined by the validator
DLL; the reference TokenValidator.dll issues single-use tokens valid ~60 s.

A client that does not want reconnect support simply selects subtype 300
instead of 301.

## 7. Validator DLL contract

The server loads the DLL named by the `VeNCryptTokenDLL` setting and resolves
these exports (`extern "C"`, `__cdecl`):

```c
int  validate_token(const char* user, const char* token);   /* !=0 = valid   */
int  issue_session_token(const char* user, char* out, size_t outSize);
int  tv_get_capabilities(void);            /* bitmask below                */
int  tv_has_management_ui(void);
void tv_show_management_ui(HWND parent);
int  tv_get_last_error(char* buf, size_t bufSize);           /* optional     */
```

Capability bits returned by `tv_get_capabilities()`:

```
TV_CAP_SESSION_TOKEN  0x01   subtype 301 advertised; DLL issues + accepts
                             session tokens in validate_token()
TV_CAP_MANAGEMENT_UI  0x02   the WinVNC "Manage users..." button is shown
                             and calls tv_show_management_ui()
```

Only `validate_token` is mandatory. A DLL exporting nothing else yields plain
subtype-300 token auth (no session tokens, no UI).

## 8. Interoperability guide

* **Fastest path for a third-party client:** speak standard VeNCrypt and pick
  subtype 259 (TLSPlain) or 262 (X509Plain). Read the spec-defined `0x01` Ack
  after sending the subtype (section 3), run TLS, then send the
  standard `userLen/passLen/user/pass` message with the token in the `pass`
  field. Everything else is textbook RFB.
* **Full compatibility (incl. reconnect):** implement subtype 301 as in
  sections 5–6 — ~50 lines beyond TLSPlain.
* **Not supported:** RealVNC's RSA-AES auth, noVNC/websockify (no VeNCrypt),
  viewers lacking a VeNCrypt stack.
* **Token size:** 64 ASCII bytes max. Large credentials (JWT/OIDC tokens)
  cannot pass through this protocol; they would require a new subtype.

## 9. Reference implementation

| Side   | Files |
|--------|-------|
| Server | `winvnc/winvnc/VeNCryptAuth.{h,cpp}`, `winvnc/winvnc/SchannelTLS.{h,cpp}`, `winvnc/winvnc/vncclient.cpp` (`AuthVeNCrypt`) |
| Viewer | `vncviewer/ClientConnectionTLS.cpp` (`AuthVeNCrypt`, `TLSSession`), `vncviewer/ClientConnection.cpp` |
| DLL API| `TokenValidator/TokenValidatorPlugin.h` (MIT-licensed for proprietary validators), `TokenValidator/CustomValidatorSkeleton.cpp` |
| Probes | `TokenValidator/test_vencrypt*.py` — minimal Python client handshake examples |
