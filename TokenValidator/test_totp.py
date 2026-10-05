import socket, ssl, struct, sys, time, base64, hmac, hashlib

HOST = "127.0.0.1"
PORT = 5900
USER = b"test"
SECRET_BASE32 = sys.argv[1].encode() if len(sys.argv) > 1 else b"JBSWY3DPEHPK3PXP"

def current_totp(secret_b32):
    key = base64.b32decode(secret_b32)
    t = int(time.time()) // 30
    c = struct.pack(">Q", t)
    h = hmac.new(key, c, hashlib.sha1).digest()
    o = h[-1] & 15
    code = (struct.unpack(">I", h[o:o+4])[0] & 0x7FFFFFFF) % 1000000
    return ("%06d" % code).encode()

TOKEN = current_totp(SECRET_BASE32)
print("using TOTP code:", TOKEN.decode())

s = socket.create_connection((HOST, PORT), timeout=10)
s.settimeout(10)

def rd(n):
    buf = b""
    while len(buf) < n:
        chunk = s.recv(n - len(buf))
        if not chunk:
            raise Exception("connection closed, got %d of %d" % (len(buf), n))
        buf += chunk
    return buf

sv = rd(12)
print("server version:", sv)
s.sendall(b"RFB 003.008\n")

for round_ in range(4):
    count = rd(1)[0]
    types = list(rd(count))
    print("round %d auth types:" % round_, types)
    if 19 in types:
        s.sendall(bytes([19]))
        break
    elif 17 in types:
        s.sendall(bytes([17]))
        res = struct.unpack(">I", rd(4))[0]
        print("auth result round %d: 0x%08X" % (round_, res))
        continue
    else:
        sys.exit("no VeNCrypt offered")

ver = rd(2)
print("VeNCrypt version:", ver.hex())
s.sendall(b"\x00\x02")
ack = rd(1)[0]
print("version ack:", ack)
if ack != 0:
    sys.exit("server rejected version")

nsub = rd(1)[0]
subs = [struct.unpack(">I", rd(4))[0] for _ in range(nsub)]
print("subtypes:", subs)
# Use X509Plain (262) to mimic TigerVNC standard path; fallbacks: TLSPlain (259), TokenTLS (300).
subtype = 262 if 262 in subs else (259 if 259 in subs else 300)
print("choosing subtype:", subtype)
s.sendall(struct.pack(">I", subtype))
ok = rd(1)[0]
print("subtype ack:", ok)
if ok != 1:
    sys.exit("server rejected subtype")

ctx = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)
ctx.check_hostname = False
ctx.verify_mode = ssl.CERT_NONE
ctx.minimum_version = ssl.TLSVersion.TLSv1_2
tls = ctx.wrap_socket(s, server_hostname="localhost", do_handshake_on_connect=True)
print("TLS established:", tls.version(), tls.cipher())

def trd(n):
    buf = b""
    while len(buf) < n:
        chunk = tls.recv(n - len(buf))
        if not chunk:
            raise Exception("tls closed, got %d of %d" % (len(buf), n))
        buf += chunk
    return buf

msg = struct.pack(">I", len(USER)) + struct.pack(">I", len(TOKEN)) + USER + TOKEN
tls.sendall(msg)
print("credentials sent")

res = struct.unpack(">I", trd(4))[0]
print("auth result:", res, {0: "OK", 1: "FAILED", 2: "TOO MANY", 0xFFFFFFFF: "CONTINUE"}.get(res, "?"))
if res != 0:
    ln = struct.unpack(">I", trd(4))[0]
    print("reason:", trd(ln))
    sys.exit(1)

# keep session alive briefly
tls.sendall(b"\x01")  # ClientInit shared
time.sleep(0.2)
print("TOTP authentication succeeded")
tls.close()
