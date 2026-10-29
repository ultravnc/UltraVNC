# Test client for winvnc RA2 / RA2ne (RealVNC RSA-AES security types).
# Mirrors the wire protocol in vncviewer/ClientConnectionRSAAES.cpp and
# TigerVNC common/rfb/SSecurityRSAAES.cxx / noVNC core/ra2.js.
#
# Usage: python tests/ra2_client.py [subtype]
#   subtype: 5 (RA2), 129 (RA2_256), 6 (RA2ne), 130 (RA2ne_256)

import socket, struct, sys, hashlib
from Crypto.PublicKey import RSA
from Crypto.Cipher import AES, PKCS1_v1_5, DES

HOST, PORT = "127.0.0.1", 5900
PASSWORD = "test1234"

SECTYPE = {5: 128, 6: 128, 129: 256, 130: 256}
ALLENC = {5: True, 129: True, 6: False, 130: False}


def recvn(sock, n):
    buf = b""
    while len(buf) < n:
        chunk = sock.recv(n - len(buf))
        if not chunk:
            raise RuntimeError("connection closed")
        buf += chunk
    return buf


class Eax:
    def __init__(self, key):
        self.key = key
        self.counter = 0

    def frame(self, plain):
        nonce = self.counter.to_bytes(16, "little")
        self.counter += 1
        aad = len(plain).to_bytes(2, "big")
        c = AES.new(self.key, AES.MODE_EAX, nonce=nonce)
        c.update(aad)
        ct, tag = c.encrypt_and_digest(plain)
        return aad + ct + tag

    def open(self, frame):
        aad, ct_tag = frame[:2], frame[2:]
        size = int.from_bytes(aad, "big")
        assert len(ct_tag) == size + 16
        nonce = self.counter.to_bytes(16, "little")
        self.counter += 1
        c = AES.new(self.key, AES.MODE_EAX, nonce=nonce)
        c.update(aad)
        try:
            return c.decrypt_and_verify(ct_tag[:size], ct_tag[size:])
        except ValueError:
            raise AssertionError("EAX tag mismatch")


def recv_frame(sock, eax):
    aad = recvn(sock, 2)
    size = int.from_bytes(aad, "big")
    rest = recvn(sock, size + 16)
    return eax.open(aad + rest)


def main(sectype):
    keybits = SECTYPE[sectype]
    allenc = ALLENC[sectype]
    hashfn = hashlib.sha1 if keybits == 128 else hashlib.sha256

    s = socket.create_connection((HOST, PORT), 10)
    server_ver = recvn(s, 12)
    print("server version:", server_ver)
    s.sendall(b"RFB 003.008\n")

    ntypes = recvn(s, 1)[0]
    types = recvn(s, ntypes)
    print("security types:", list(types))
    assert sectype in types, f"type {sectype} not offered"
    s.sendall(bytes([sectype]))

    # 1. server public key: u32 bitlen | n | e  (big-endian, padded)
    bitlen = struct.unpack(">I", recvn(s, 4))[0]
    kb = (bitlen + 7) // 8
    server_n = recvn(s, kb)
    server_e = recvn(s, kb)
    print(f"server RSA key {bitlen} bits")
    server_pub = RSA.construct((int.from_bytes(server_n, "big"),
                                int.from_bytes(server_e, "big")))

    # 2. client public key
    client_key = RSA.generate(2048)
    ckb = (client_key.size_in_bits() + 7) // 8
    client_n = client_key.n.to_bytes(ckb, "big")
    client_e = client_key.e.to_bytes(ckb, "big")
    s.sendall(struct.pack(">I", client_key.size_in_bits()) + client_n + client_e)

    # 3. randoms (client first per ra2.js, then server)
    client_random = bytes(keybits // 8)
    ct = PKCS1_v1_5.new(server_pub).encrypt(client_random)
    s.sendall(struct.pack(">H", len(ct)) + ct)
    ln = struct.unpack(">H", recvn(s, 2))[0]
    server_random = PKCS1_v1_5.new(client_key).decrypt(recvn(s, ln), None)
    assert server_random is not None and len(server_random) == keybits // 8, \
        "bad server random"

    # 4. session keys + EAX ciphers
    send_key = hashfn(server_random + client_random).digest()[: keybits // 8]
    recv_key = hashfn(client_random + server_random).digest()[: keybits // 8]
    enc, dec = Eax(send_key), Eax(recv_key)

    # 5. hash exchange: client sends H(lenC|cN|cE|lenS|sN|sE)
    s_pub = struct.pack(">I", bitlen) + server_n + server_e
    c_pub = struct.pack(">I", client_key.size_in_bits()) + client_n + client_e
    client_hash = hashfn(c_pub + s_pub).digest()
    s.sendall(enc.frame(client_hash))
    server_hash = recv_frame(s, dec)
    assert server_hash == hashfn(s_pub + c_pub).digest(), "server hash mismatch"
    print("key hash OK")

    # 6. subtype (encrypted)
    subtype = recv_frame(s, dec)[0]
    print("auth subtype:", subtype)  # 1=UserPass 2=Pass

    # 7. credentials (single frame is fine; server reads a byte stream)
    user = b""
    cred = bytes([len(user)]) + user + bytes([len(PASSWORD)]) + PASSWORD.encode()
    s.sendall(enc.frame(cred))

    # 8. security result: EAX-framed for RA2, plain u32 for RA2ne
    if allenc:
        result = struct.unpack(">I", recv_frame(s, dec))[0]
    else:
        result = struct.unpack(">I", recvn(s, 4))[0]
    print("security result:", result, "(0 = OK)")
    if result == 0:
        # ClientInit (1 byte shared flag), then read ServerInit:
        # u16 w | u16 h | pixel-format 16 | u32 name-len | name
        if allenc:
            s.sendall(enc.frame(b"\x01"))  # ClientInit, EAX-framed
        else:
            s.sendall(b"\x01")             # ClientInit, plaintext
        if allenc:
            stream = b""
            while len(stream) < 24:
                stream += recv_frame(s, dec)
            name_len = struct.unpack(">I", stream[20:24])[0]
            while len(stream) < 24 + name_len:
                stream += recv_frame(s, dec)
            print("server init:", stream[:24].hex(), "name:", stream[24:24 + name_len])
        else:
            hdr = recvn(s, 24)
            name_len = struct.unpack(">I", hdr[20:24])[0]
            print("server init:", hdr.hex(), "name:", recvn(s, name_len))
    s.close()
    return result == 0


if __name__ == "__main__":
    st = int(sys.argv[1]) if len(sys.argv) > 1 else 129
    ok = main(st)
    sys.exit(0 if ok else 1)
