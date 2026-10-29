#!/usr/bin/env python3
"""Fake AppleDH (RFB security type 30) server for testing vncviewer.

Implements the Apple Remote Desktop / macOS Screen Sharing auth:
  S->C: u16 generator | u16 keysize | modulus[keysize] | server_pubkey[keysize]
  C->S: AES-128-ECB(MD5(dh_shared), 64B user || 64B pass) + client_pubkey[keysize]

Usage: python appledh_server.py [port] [expected_user] [expected_pass]
"""
import hashlib
import socket
import struct
import sys
import threading

from Crypto.Cipher import AES

# RFC 3526 group 2 - 1024-bit MODP prime (Apple uses a fixed 1024-bit group)
PRIME = int(
    "FFFFFFFFFFFFFFFFC90FDAA22168C234C4C6628B80DC1CD1"
    "29024E088A67CC74020BBEA63B139B22514A08798E3404DD"
    "EF9519B3CD3A431B302B0A6DF25F14374FE1356D6D51C245"
    "E485B576625E7EC6F44C42E9A637ED6B0BFF5CB6F406B7ED"
    "EE386BFB5A899FA5AE9F24117C4B1FE649286651ECE65381"
    "FFFFFFFFFFFFFFFF", 16)
GEN = 2
KEYSIZE = 128
PORT = int(sys.argv[1]) if len(sys.argv) > 1 else 5901
EXP_USER = (sys.argv[2] if len(sys.argv) > 2 else "appleuser").encode()
EXP_PASS = (sys.argv[3] if len(sys.argv) > 3 else "applepass").encode()

# server DH secret
y = 0xC0FFEE12345
server_pub = pow(GEN, y, PRIME)


def i2b(n):
    return n.to_bytes(KEYSIZE, "big")


def recv_exact(c, n):
    b = b""
    while len(b) < n:
        d = c.recv(n - len(b))
        if not d:
            raise EOFError("closed")
        b += d
    return b


def handle(c, addr):
    try:
        c.sendall(b"RFB 003.008\n")
        ver = recv_exact(c, 12)
        print(f"client version: {ver!r}")
        c.sendall(bytes([1, 30]))  # offer only AppleDH
        sel = recv_exact(c, 1)[0]
        print(f"client selected security type {sel}")
        assert sel == 30, "viewer did not pick AppleDH"

        print(f"server sends pub head: {i2b(server_pub)[:16].hex()}")
        c.sendall(struct.pack(">HH", GEN, KEYSIZE) + i2b(PRIME) + i2b(server_pub))

        cred = recv_exact(c, 128)
        cpub_bytes = recv_exact(c, KEYSIZE)
        print(f"server got client_pub head: {cpub_bytes[:16].hex()}")
        client_pub = int.from_bytes(cpub_bytes, "big")
        shared = pow(client_pub, y, PRIME)
        print(f"server secret head: {shared.to_bytes(KEYSIZE, 'big')[:16].hex()} tail: {shared.to_bytes(KEYSIZE, 'big')[-16:].hex()}")
        key = hashlib.md5(shared.to_bytes(KEYSIZE, "big")).digest()
        plain = AES.new(key, AES.MODE_ECB).decrypt(cred)
        user = plain[:64].split(b"\0")[0]
        passwd = plain[64:].split(b"\0")[0]
        print(f"decrypted credentials: user={user!r} pass={passwd!r}")

        ok = user == EXP_USER and passwd == EXP_PASS
        c.sendall(struct.pack(">I", 0 if ok else 1))
        print(f"auth {'OK' if ok else 'FAILED'}")

        ci = recv_exact(c, 1)
        print(f"ClientInit: {ci[0]}")
        # minimal ServerInit: 1024x768, pixel format, name
        pf = bytes([32, 24, 0, 1]) + struct.pack(">HHHBBB", 255, 255, 255, 16, 8, 0) + b"\x00\x00\x00"
        name = b"appledh-fake"
        c.sendall(struct.pack(">HH", 1024, 768) + pf + struct.pack(">I", len(name)) + name)
        print("ServerInit sent - handshake complete")
    except Exception as e:
        print(f"error: {e}")
    finally:
        c.close()


s = socket.socket()
s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
s.bind(("127.0.0.1", PORT))
s.listen(4)
print(f"fake AppleDH server on 127.0.0.1:{PORT} (expecting user={EXP_USER!r} pass={EXP_PASS!r})")
while True:
    c, a = s.accept()
    threading.Thread(target=handle, args=(c, a), daemon=True).start()
