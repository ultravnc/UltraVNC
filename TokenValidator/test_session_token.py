#!/usr/bin/env python3
import socket, struct, ssl, time, hmac, hashlib, base64

HOST = "127.0.0.1"
PORT = 5900
USER = b"debug"
SECRET = "LT4O3H3N4SECUUXWJTXLCQIRX3SYRBBI"

def totp_code():
    key = base64.b32decode(SECRET)
    t = int(time.time()) // 30
    msg = struct.pack(">Q", t)
    d = hmac.new(key, msg, hashlib.sha1).digest()
    o = d[19] & 15
    code = (struct.unpack(">I", d[o:o+4])[0] & 0x7FFFFFFF) % 1000000
    return f"{code:06d}".encode()

def rd(sock, n):
    buf = b""
    while len(buf) < n:
        chunk = sock.recv(n - len(buf))
        if not chunk:
            raise Exception(f"closed, got {len(buf)} of {n}")
        buf += chunk
    return buf

def connect_and_auth(use_session_token=None):
    s = socket.create_connection((HOST, PORT), timeout=10)
    # RFB version
    sv = rd(s, 12)
    s.sendall(b"RFB 003.008\n")

    # auth list
    count = rd(s, 1)[0]
    types = list(rd(s, count))
    print("auth types:", types)
    if 19 not in types:
        raise Exception("VeNCrypt not offered")
    s.sendall(bytes([19]))

    # VeNCrypt version
    ver = rd(s, 2)
    print("VeNCrypt version:", ver.hex())
    s.sendall(b"\x00\x02")
    ok = rd(s, 1)[0]
    if ok != 0:
        raise Exception(f"VeNCrypt version rejected: {ok}")

    # subtypes
    st_count = rd(s, 1)[0]
    st = struct.unpack(">" + str(st_count) + "I", rd(s, 4 * st_count))
    print("subtypes:", st)
    if 301 not in st:
        raise Exception("TokenTLSv2 not offered")
    s.sendall(struct.pack(">I", 301))
    subtype_ok = rd(s, 1)[0]
    if subtype_ok != 1:
        raise Exception(f"subtype rejected: {subtype_ok}")

    # TLS handshake
    ctx = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)
    ctx.check_hostname = False
    ctx.verify_mode = ssl.CERT_NONE
    ctx.minimum_version = ssl.TLSVersion.TLSv1_2
    tls = ctx.wrap_socket(s, server_hostname=HOST)
    print("TLS established:", tls.version())

    # credentials
    if use_session_token:
        token = use_session_token
        print("sending session token")
    else:
        token = totp_code()
        print("sending TOTP:", token)
    tls.sendall(struct.pack(">I", len(USER)))
    tls.sendall(struct.pack(">I", len(token)))
    tls.sendall(USER)
    tls.sendall(token)

    # auth result
    res = struct.unpack(">I", rd(tls, 4))[0]
    print("auth result:", res)
    if res != 0:
        raise Exception("auth failed")

    # session token (TokenTLSv2)
    if use_session_token is None:
        tok_len = rd(tls, 1)[0]
        session_token = rd(tls, tok_len)
        print("received session token:", session_token)
        return session_token
    else:
        return None

if __name__ == "__main__":
    print("--- first connect with TOTP ---")
    tok = connect_and_auth()

    print("\n--- reconnect with session token ---")
    connect_and_auth(use_session_token=tok)
    print("reconnect succeeded")

    print("\n--- reuse old session token after it was revoked ---")
    try:
        connect_and_auth(use_session_token=tok)
        print("ERROR: old token was accepted")
    except Exception as e:
        print("old token correctly rejected:", e)
