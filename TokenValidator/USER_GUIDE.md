# Encrypted VNC with token login — User guide

This guide explains, step by step, how to turn on **encrypted VNC** (TLS 1.2
or TLS 1.3) and log in with a **username + 6-digit code** from an
authenticator app — the same kind of code banks use.

No prior knowledge needed. Just follow the steps for your role:

* I am the **server admin** (the PC people connect *to*) → Part 1
* I am a **viewer user** (the PC that connects) → Part 2

---

## Part 1 — Server admin: set it up (one time, ~5 minutes)

### Step 1 — Turn on encryption

1. Right-click the **UltraVNC server icon** in the system tray → **Admin Properties**.
2. Open the **SSL/TLS** tab.
3. Tick **Enable VeNCrypt TLS**.
4. (Recommended) Tick **Require TLS** so old-style unencrypted logins are refused.
5. Click **OK**. Restart the server if asked.

### Step 2 — Tell the server which checker to use

The "checker" is a small file (`TokenValidator.dll`) that decides whether a
login code is correct.

1. Same **SSL/TLS** tab → field **Token validator DLL**.
2. If `TokenValidator.dll` is in the same folder as `winvnc.exe`, the path
   is filled in automatically. Otherwise click **Browse...** and select it
   (for example `C:\uvnc\64\xp\TokenValidator.dll`).

   The path must be absolute; relative paths are rejected for security.

### Step 3 — Create your first user

1. Still on the SSL/TLS tab, click **Manage users...**
   *(if the button is grey, the DLL path in step 2 is missing, not absolute, or
   the DLL could not be loaded).*
2. Click **Add**, type a username (e.g. `rudi`), OK.
3. A **QR code** appears. Leave this window open for Part 3.

   *Alternative without the button:* run `TokenSetup.exe rudi` on the server.
   It prints a secret and writes a `rudi_totp.bmp` QR image next to it.

### Step 4 — Done

The server now accepts encrypted connections only. Tell the viewer user their
**username** — never send anyone the QR code or secret by chat/email unless the
channel is trusted: **whoever has the QR code can log in.**

---

## Part 2 — Viewer user: get the app + code (one time, ~2 minutes)

### Step 1 — Install an authenticator app (phone)

Any of these — pick one, all free:

* Microsoft Authenticator
* Google Authenticator
* Authy

### Step 2 — Scan the QR code

1. Open the app → **+ (add account)** → **scan QR code**.
2. Point the camera at the QR code the admin shows you
   (screen or the `*_totp.bmp` image).
3. The app now shows **"UltraVNC: yourname"** with a **6-digit code** that
   changes every 30 seconds. That's it — setup done.

   *(No camera? The admin can give you the "base32 secret" text to type in.)*

---

## Part 3 — Connecting (every time)

1. Open **UltraVNC Viewer**, type the server address, click **Connect**.
2. A login box appears asking for **username + token**:
   * **Username** = the name the admin created (e.g. `rudi`)
   * **Token** = the 6-digit code currently showing in your app
3. Click OK. You're in — the whole session is now encrypted (TLS), including
   file transfer.

### "My code keeps changing — which one do I use?"

The code on screen **right now** when you click OK. It lives for ~30 seconds;
codes just before/after also still work (clock-slack). If it flips mid-typing,
just use the new one.

### Reconnecting quickly

If the connection drops (e.g. Windows logon screen switch) and you reconnect,
the viewer remembers a one-time "session token" and you **don't need a new
code** — it just connects. The token is valid for up to 12 hours and is consumed
on use; a successful reconnect immediately receives a replacement token.
Tokens are never saved to disk.

---

## Something's wrong?

| Symptom | Probably means | Fix |
|---|---|---|
| "Authentication failed" | Wrong username, or mistyped/expired code | Check the username with admin; wait for a fresh code and retype carefully |
| Connect works without any code prompt | Server isn't offering token-TLS | Admin: check **Enable VeNCrypt TLS** is ticked and the DLL path is set |
| "Manage users..." button is grey | DLL not found at the configured path | Admin: re-browse to `TokenValidator.dll` |
| User added but codes always rejected | Server clock is wrong | TOTP is time-based — fix the clock/timezone on the server |
| Login worked before, now always fails | Secret was reset, or you scan the wrong entry | Re-scan the new QR; delete the old "UltraVNC" entry in your app |
| "TLS handshake failed" | Certificate problem on the server | Admin: SSL/TLS tab → **New self-signed** to regenerate |

## Notes for the admin

* Secrets are stored on the server in **Windows Credential Manager**
  (`UltraVNC_TOTP_<user>`) plus an encrypted `TokenValidator.credentials.ini`
  fallback for service accounts — both stay on the server, never sent anywhere.
* Each user needs their **own** username + their own phone app — codes are
  per-secret, so two people can't share one account usefully.
* The session token is random and 64 characters. The viewer keeps it in memory;
  the server stores a DPAPI-protected record for at most 12 hours. It is consumed
  on use and replaced after a successful reconnect.
* Connections use TLS 1.2 as a minimum; TLS 1.3 is negotiated automatically
  on Windows 11 / Windows Server 2022 and later.
* Full technical detail (DLL ABI, custom validators, build): see `README.md`
  in this folder.
