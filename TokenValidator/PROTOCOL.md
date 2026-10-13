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
response"*) and is implemented by all major stacks — in TigerVNC/KasmVNC it
lives in the TLS sub-security layer (`CSecurityTLS`/`SSecurityTLS`), not in
the VeNCrypt negotiation class. The UltraVNC viewer additionally tolerates
a *missing* Ack: it waits briefly and proceeds to TLS if none arrives, so
it also works against hypothetical old-draft servers.

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
| 300   | secTypeTokenTLS   | private versioned UltraVNC token protocol     |

Subtype 300 is an UltraVNC private extension, not an assigned VeNCrypt subtype.
It is identified by an encrypted `UVNC` marker and protocol version. Primary
and reconnect credentials are methods inside this single subtype.

The `TLS*`/`X509*` pairs behave identically in this implementation — the
certificate is always presented and the client is expected to verify it (the
UltraVNC viewer shows a warning dialog with certificate details and thumbprint
persistence on verification failure).

## 5. Authentication request

### Standard subtypes 259 and 262

These retain the standard VeNCrypt Plain layout:

```
C->S  u32 userLen + u32 passwordLen + user + password
```

### Private subtype 300

Immediately after TLS, the client sends:

```
C->S  char[4] magic       "UVNC" (bytes 55 56 4e 43)
C->S  u8 major            1
C->S  u8 minor            0
C->S  u8 method           1 = primary token, 2 = reconnect session token
C->S  u8 flags            bit 0 = request a new session token on success
C->S  u32 userLen         1..256
C->S  u32 credentialLen   6..64
C->S  userLen bytes       username (UTF-8/ASCII)
C->S  credentialLen bytes opaque credential
```

Unknown major versions, methods or flag bits are rejected. A server may accept
a lower-or-equal minor version within major version 1. Token semantics remain
the validator DLL's responsibility.

For standard Plain subtypes (259/262), credentials may be 0..1024 bytes so
generic clients reach `validate_token`; the DLL decides validity.

## 6. SecurityResult and session token

Standard RFB SecurityResult, inside the TLS channel:

```
S->C  u32 result       0 = OK, 1 = failed
      if failed:
S->C  u32 reasonLen + reasonLen bytes   UTF-8 reason string
```

On `result = 0` the client proceeds to `ClientInit` and the normal RFB session
— framebuffer, input, file transfer — all inside TLS.

For subtype 300, when request flag bit 0 was set, immediately after
`SecurityResult = 0` the server sends:

```
S->C  u8 responseFlags    bit 0 = session token present
S->C  u16 tokenLen        big-endian; currently 0..64
S->C  tokenLen bytes      present only when responseFlags bit 0 is set
```

The response header is always sent when requested. Flags=0 and tokenLen=0 mean
that issuance is unsupported or failed; authentication remains successful.
A reconnect sends method 2 and may request another token. Lifetime and
acceptance policy are defined by the validator DLL. The reference DLL stores
one machine-protected pending token per user until it is used or replaced.
Every successful reconnect immediately issues a replacement, and a service
desktop-process restart does not invalidate the pending token.

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
TV_CAP_SESSION_TOKEN  0x01   DLL can issue and accept reconnect session tokens;
                             subtype 300 remains advertised either way
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
* **Full compatibility (including reconnect):** implement the versioned
  subtype-300 request and response in sections 5–6.
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
