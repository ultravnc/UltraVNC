// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
//

// Server-side RSA-AES (RA2 / RA2ne) authentication.
// Wire protocol mirrors TigerVNC common/rfb/SSecurityRSAAES.cxx and
// noVNC core/ra2.js; the AES-EAX framing matches the viewer's
// ClientConnectionRSAAES.cpp.

#include "stdhdrs.h"
#include "RSAAESAuth.h"
#include "SchannelTLS.h"
#include "SettingsManager.h"
#include "vnclog.h"

// Older SDKs may not define the CNG PKCS1 padding flag.
#ifndef NCRYPT_PAD_PKCS1
#define NCRYPT_PAD_PKCS1 0x00000002
#endif

extern VNCLog vnclog;

static const int secTypeRA2None = 0;
static const int secTypeRA2UserPass = 1;
static const int secTypeRA2Pass = 2;

static const DWORD MinRsaKeyLength = 1024;
static const DWORD MaxRsaKeyLength = 8192;
static const DWORD MaxRsaKeyBytes = MaxRsaKeyLength / 8;

static void ArraySwap(BYTE* p, DWORD size)
{
    for (DWORD i = 0; i < size / 2; i++)
    {
        const BYTE t = p[i];
        p[i] = p[size - 1 - i];
        p[size - 1 - i] = t;
    }
}

static bool ArrayEqual(const BYTE* a, const BYTE* b, DWORD size)
{
    BYTE r = 0;
    for (DWORD i = 0; i < size; i++)
        r |= a[i] ^ b[i];
    return r == 0;
}

// AES cipher as wrapper for Crypto API forward permutation (same as viewer).

struct AESCipher
{
    static const DWORD BlockSize = 16;

    struct SymKeyBlob
    {
        BLOBHEADER  hdr;
        DWORD       cbKeySize;
        BYTE        key[32];
    };

    HCRYPTPROV  hProv;
    HCRYPTKEY   hKey;
    char        err[256];

    AESCipher() : hProv(0), hKey(0) { err[0] = 0; }

    ~AESCipher()
    {
        if (hKey)
            CryptDestroyKey(hKey);
        if (hProv)
            CryptReleaseContext(hProv, 0);
    }

    bool Init(DWORD keySize, void* key)
    {
        if (!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT))
            return SetErr("CryptAcquireContext failed (%u)", GetLastError());
        SymKeyBlob keyBlob = { 0 };
        keyBlob.hdr.bType = PLAINTEXTKEYBLOB;
        keyBlob.hdr.bVersion = CUR_BLOB_VERSION;
        keyBlob.hdr.aiKeyAlg = (keySize == 128 ? CALG_AES_128 : CALG_AES_256);
        keyBlob.cbKeySize = min(keySize / 8, (DWORD)sizeof(keyBlob.key));
        memcpy(keyBlob.key, key, keyBlob.cbKeySize);
        if (!CryptImportKey(hProv, (BYTE*)&keyBlob, offsetof(SymKeyBlob, key) + keyBlob.cbKeySize, NULL, 0, &hKey))
            return SetErr("CryptImportKey failed (%u)", GetLastError());
        DWORD mode = CRYPT_MODE_ECB;
        if (!CryptSetKeyParam(hKey, KP_MODE, (BYTE*)&mode, 0))
            return SetErr("CryptSetKeyParam failed (%u)", GetLastError());
        return true;
    }

    bool Encrypt(void* buf, DWORD size = BlockSize)
    {
        if (!CryptEncrypt(hKey, NULL, FALSE, 0, (BYTE*)buf, &size, size))
            return SetErr("CryptEncrypt failed (%u)", GetLastError());
        return true;
    }

private:
    bool SetErr(const char* fmt, DWORD e)
    {
        snprintf(err, sizeof(err), fmt, e);
        return false;
    }
};

// CMAC authenticator (same algorithm as the viewer's CMACAuth).

struct CMACAuth
{
    static const DWORD BlockSize = AESCipher::BlockSize;

    AESCipher* cipher;
    BYTE       k1[BlockSize], k2[BlockSize];
    BYTE       pad[BlockSize];
    DWORD      pos;

    bool Init(AESCipher& c)
    {
        cipher = &c;
        Reset();
        if (!cipher->Encrypt(pad))
            return false;
        DoubleBlock(pad, k1);
        DoubleBlock(k1, k2);
        Reset();
        return true;
    }

    void Reset()
    {
        memset(pad, 0, sizeof(pad));
        pos = 0;
    }

    bool Process(const void* buf, DWORD size)
    {
        for (DWORD i = 0; i < size; i++)
        {
            if (pos == BlockSize)
            {
                if (!cipher->Encrypt(pad))
                    return false;
                pos = 0;
            }
            pad[pos++] ^= ((const BYTE*)buf)[i];
        }
        return true;
    }

    bool Finalize(void* tag, DWORD size = BlockSize)
    {
        if (pos < BlockSize)
            pad[pos] ^= 0x80;
        const BYTE* key = (pos < BlockSize ? k2 : k1);
        for (DWORD i = 0; i < BlockSize; i++)
            pad[i] ^= key[i];
        if (!cipher->Encrypt(pad))
            return false;
        pos = 0;
        memcpy(tag, pad, size);
        return true;
    }

private:
    static void DoubleBlock(BYTE* in, BYTE* out)
    {
        DWORD c = 0;
        for (int i = BlockSize - 1; i >= 0; i--)
        {
            DWORD t = in[i];
            out[i] = (BYTE)((t << 1) | c);
            c = ((t & 0x80) != 0);
        }
        out[BlockSize - 1] ^= c * 0x87;
    }
};

// EAX mode AEAD (same as the viewer's EAXMode).

struct EAXMode
{
    static const DWORD BlockSize = AESCipher::BlockSize;
    static const DWORD MacSize = CMACAuth::BlockSize;

    AESCipher   cipher;
    CMACAuth    auth;
    BYTE        ctr[BlockSize], pad[BlockSize];
    int         pos;
    BYTE        tagNonce[MacSize];
    BYTE        tagAad[MacSize];

    bool Init(DWORD keySize, void* key, char* errBuf, size_t errSize)
    {
        errBuf[0] = 0;
        if (!cipher.Init(keySize, key))
        {
            snprintf(errBuf, errSize, "%s", cipher.err);
            return false;
        }
        if (!auth.Init(cipher))
        {
            snprintf(errBuf, errSize, "%s", cipher.err);
            return false;
        }
        pos = -1;
        return true;
    }

    bool SetNonce(DWORD msg)
    {
        BYTE buf[BlockSize] = { 0 };
        memcpy(buf, &msg, 4);
        return SetNonce(buf, BlockSize);
    }

    bool SetNonce(const void* buf, DWORD size)
    {
        if (!Mac(0, buf, size, tagNonce))
            return false;
        memcpy(ctr, tagNonce, sizeof(ctr));
        return true;
    }

    bool SetAad(const void* buf, DWORD size)
    {
        return Mac(1, buf, size, tagAad);
    }

    bool Process(void* buf, DWORD size, bool encrypt = true)
    {
        if (pos < 0)
        {
            if (!Mac(2, NULL, 0, NULL))
                return false;
            pos = 0;
        }
        if (!encrypt && !auth.Process(buf, size))
            return false;
        for (DWORD i = 0; i < size; i++)
        {
            if (pos == 0 || pos == BlockSize)
            {
                memcpy(pad, ctr, BlockSize);
                if (!cipher.Encrypt(pad))
                    return false;
                WrapIncr(ctr);
                pos = 0;
            }
            ((BYTE*)buf)[i] ^= pad[pos++];
        }
        if (encrypt && !auth.Process(buf, size))
            return false;
        return true;
    }

    bool Finalize(void* tag, DWORD size = MacSize)
    {
        if (!auth.Finalize(tag, size))
            return false;
        for (DWORD i = 0; i < size; i++)
            ((BYTE*)tag)[i] ^= tagNonce[i] ^ tagAad[i];
        pos = -1;
        return true;
    }

private:
    bool Mac(BYTE step, const void* buf, DWORD size, void* tag)
    {
        BYTE block[BlockSize] = { 0 };
        block[BlockSize - 1] = step;
        auth.Reset();
        if (!auth.Process(block, sizeof(block)))
            return false;
        if (size && !auth.Process(buf, size))
            return false;
        if (tag)
            return auth.Finalize(tag, MacSize);
        return true;
    }

    static void WrapIncr(BYTE ctr[BlockSize])
    {
        for (int i = BlockSize - 1; i >= 0; i--)
            if (++ctr[i])
                break;
    }
};

// EAX message framing on the raw socket: u16be length (AAD) + ciphertext +
// 16-byte tag, nonce = per-direction little-endian message counter.

struct EAXStream
{
    static const DWORD AadSize = 2;
    static const DWORD MacSize = EAXMode::MacSize;

    EAXMode     encAead, decAead;
    DWORD       encMsg, decMsg;
    std::vector<BYTE> plain;
    char        err[256];

    EAXStream() : encMsg(0), decMsg(0) { err[0] = 0; }

    bool Init(DWORD keySize, void* encKey, void* decKey)
    {
        if (!encAead.Init(keySize, encKey, err, sizeof(err)))
            return false;
        if (!decAead.Init(keySize, decKey, err, sizeof(err)))
            return false;
        return true;
    }

    // Returns a complete EAX frame for `size` plaintext bytes.
    bool MakeMessage(const void* data, DWORD size, std::vector<BYTE>& out)
    {
        out.resize(AadSize + size + MacSize);
        out[0] = (BYTE)(size >> 8);
        out[1] = (BYTE)(size & 0xff);
        memcpy(out.data() + AadSize, data, size);
        if (!encAead.SetNonce(encMsg++))
            return SetErr("SetNonce failed");
        if (!encAead.SetAad(out.data(), AadSize))
            return SetErr("SetAad failed");
        if (!encAead.Process(out.data() + AadSize, size))
            return SetErr("Encrypt failed");
        if (!encAead.Finalize(out.data() + AadSize + size, MacSize))
            return SetErr("Tag failed on encryption");
        return true;
    }

    // Decrypts one frame (len bytes ciphertext + tag, AAD = len).
    bool OpenMessage(BYTE* frame, DWORD size, std::vector<BYTE>& outPlain)
    {
        outPlain.assign(frame, frame + size);
        if (!decAead.SetNonce(decMsg++))
            return SetErr("SetNonce failed");
        if (!decAead.SetAad(frame - AadSize, AadSize))
            return SetErr("SetAad failed");
        if (!decAead.Process(outPlain.data(), size, false))
            return SetErr("Decrypt failed");
        BYTE tag[MacSize];
        if (!decAead.Finalize(tag, MacSize))
            return SetErr("Tag failed on decryption");
        if (!ArrayEqual(tag, frame + size, MacSize))
            return SetErr("Decryption tag does not match");
        return true;
    }

private:
    bool SetErr(const char* msg)
    {
        snprintf(err, sizeof(err), "%s", msg);
        return false;
    }
};

// Convenience: send one EAX-framed message over the callback-installed
// socket. Used before TLSCallbacks take over, so messages are built and
// sent manually on the raw socket.
bool RSAAESAuth::RawSendAll(const BYTE* data, DWORD len)
{
    DWORD sent = 0;
    while (sent < len)
    {
        int n = m_socket->Send((const char*)data + sent, (VCard)(len - sent));
        if (n <= 0)
        {
            vnclog.Print(LL_CONNERR, VNCLOG("RA2: raw send failed n=%d wsa=%d\n"), n, WSAGetLastError());
            return false;
        }
        sent += (DWORD)n;
    }
    return true;
}

bool RSAAESAuth::RawReadExact(BYTE* data, DWORD len)
{
    DWORD got = 0;
    while (got < len)
    {
        int n = m_socket->Read((char*)data + got, (int)(len - got));
        if (n <= 0)
        {
            vnclog.Print(LL_CONNERR, VNCLOG("RA2: raw recv failed n=%d wsa=%d\n"), n, WSAGetLastError());
            return false;
        }
        got += (DWORD)n;
    }
    return true;
}

bool RSAAESAuth::Fail(const char* msg)
{
    m_lastError = msg;
    vnclog.Print(LL_CONNERR, VNCLOG("RA2: %s\n"), msg);
    return false;
}

bool RSAAESAuth::Fail(const char* msg, DWORD err)
{
    char buf[256];
    snprintf(buf, sizeof(buf), "%s (%u)", msg, err);
    return Fail(buf);
}

RSAAESAuth::RSAAESAuth(VSocket* socket)
    : m_socket(socket)
    , m_keySize(0)
    , m_allEncrypted(false)
    , m_requireUser(false)
    , m_tlsLoader(nullptr)
    , m_certCtx(nullptr)
    , m_hServerKey(0)
    , m_hCertKey(0)
    , m_certKeySpec(0)
    , m_certKeyMustFree(FALSE)
    , m_hProv(0)
    , m_hClientKey(0)
    , m_hHash(0)
    , m_serverKeyBits(0)
    , m_serverKeyBytes(0)
    , m_clientKeyBits(0)
    , m_clientKeyBytes(0)
    , m_eax(nullptr)
    , m_eaxInstalled(false)
{
}

RSAAESAuth::~RSAAESAuth()
{
    if (m_socket && m_eaxInstalled)
        m_socket->SetTLSCallbacks(nullptr);
    if (m_certKeyMustFree && m_hCertKey)
    {
        if (m_certKeySpec == CERT_NCRYPT_KEY_SPEC)
            NCryptFreeObject(m_hCertKey);
        else
            CryptReleaseContext(m_hCertKey, 0);
    }
    if (m_hHash)
        CryptDestroyHash(m_hHash);
    if (m_hClientKey)
        CryptDestroyKey(m_hClientKey);
    if (m_hProv)
        CryptReleaseContext(m_hProv, 0);
    delete m_eax;
    delete m_tlsLoader;
}

bool RSAAESAuth::Authenticate(DWORD keySize, bool allEncrypted, bool requireUser,
                              std::string& outUsername, std::string& outPassword,
                              std::string& outFailureMessage)
{
    m_keySize = keySize;
    m_allEncrypted = allEncrypted;
    m_requireUser = requireUser;

    if (!LoadServerKey()
        || !SendServerPublicKey()
        || !ReadClientPublicKey()
        || !SendServerRandom()
        || !ReadClientRandom()
        || !SetCipher()
        || !SendKeyHash()
        || !ReadClientHash()
        || !SendSubtype()
        || !ReadCredentials(outUsername, outPassword))
    {
        outFailureMessage = m_lastError.empty() ? "RA2 handshake failed" : m_lastError;
        return false;
    }

    // RA2ne: the session continues unencrypted after the credential exchange.
    if (!m_allEncrypted && m_eaxInstalled)
    {
        m_socket->SetTLSCallbacks(nullptr);
        m_eaxInstalled = false;
    }
    return true;
}

// Load the VeNCrypt certificate and expose its RSA key pair for RA2.
bool RSAAESAuth::LoadServerKey()
{
    SettingsManager* settings = SettingsManager::getInstance();
    m_tlsLoader = new SchannelTLS();

    bool certOk = false;
    const char* thumbprint = settings->getVeNCryptTLSCertThumbprint();
    if (thumbprint && thumbprint[0])
        certOk = m_tlsLoader->LoadCertByThumbprint(thumbprint);
    if (!certOk)
    {
        char generatedThumbprint[41] = {};
        certOk = m_tlsLoader->EnsureSelfSignedCert("CN=UltraVNC-VeNCrypt", "UltraVNC-VeNCrypt",
                                                 generatedThumbprint, settings->getTLSCertKeyBits());
        if (certOk)
        {
            settings->setVeNCryptTLSCertThumbprint(generatedThumbprint);
            settings->save();
        }
    }
    if (!certOk)
        return Fail("failed to load or create certificate");

    PCCERT_CONTEXT ctx = m_tlsLoader->GetCertContext();
    if (!ctx)
        return Fail("no certificate context");

    // RA2 needs an RSA certificate.
    CERT_PUBLIC_KEY_INFO* pki = &ctx->pCertInfo->SubjectPublicKeyInfo;
    if (!pki->Algorithm.pszObjId || strcmp(pki->Algorithm.pszObjId, szOID_RSA_RSA) != 0)
        return Fail("certificate public key is not RSA");

    // Decode the public key (gives PUBLICKEYBLOB: header + RSAPUBKEY + LE modulus).
    DWORD blobLen = 0;
    if (!CryptDecodeObject(X509_ASN_ENCODING, RSA_CSP_PUBLICKEYBLOB,
                           pki->PublicKey.pbData, pki->PublicKey.cbData, 0, NULL, &blobLen))
        return Fail("public key decode size failed", GetLastError());
    std::vector<BYTE> blob(blobLen);
    if (!CryptDecodeObject(X509_ASN_ENCODING, RSA_CSP_PUBLICKEYBLOB,
                           pki->PublicKey.pbData, pki->PublicKey.cbData, 0, blob.data(), &blobLen))
        return Fail("public key decode failed", GetLastError());

    RSAPUBKEY* rsaPub = (RSAPUBKEY*)(blob.data() + sizeof(BLOBHEADER));
    BYTE* modulusLE = blob.data() + sizeof(BLOBHEADER) + sizeof(RSAPUBKEY);
    if (rsaPub->bitlen < MinRsaKeyLength || rsaPub->bitlen > MaxRsaKeyLength)
        return Fail("certificate RSA key size out of range");

    m_serverKeyBits = rsaPub->bitlen;
    m_serverKeyBytes = (m_serverKeyBits + 7) / 8;

    m_serverN.assign(modulusLE, modulusLE + m_serverKeyBytes);
    ArraySwap(m_serverN.data(), m_serverKeyBytes);
    m_serverE.assign(m_serverKeyBytes, 0);
    DWORD e = rsaPub->pubexp;
    for (int i = (int)m_serverKeyBytes - 1; i >= 0 && e; i--)
    {
        m_serverE[i] = (BYTE)(e & 0xff);
        e >>= 8;
    }

    // Acquire the private key handle for decrypting the client random.
    if (!CryptAcquireCertificatePrivateKey(ctx,
            CRYPT_ACQUIRE_SILENT_FLAG | CRYPT_ACQUIRE_PREFER_NCRYPT_KEY_FLAG,
            NULL, &m_hCertKey, &m_certKeySpec, &m_certKeyMustFree))
        return Fail("private key not accessible", GetLastError());
    m_hServerKey = (m_certKeySpec == CERT_NCRYPT_KEY_SPEC) ? (NCRYPT_KEY_HANDLE)m_hCertKey : 0;
    m_certCtx = ctx;
    return true;
}

bool RSAAESAuth::SendServerPublicKey()
{
    std::vector<BYTE> msg(4 + m_serverKeyBytes * 2);
    DWORD len = Swap32IfLE(m_serverKeyBits);
    memcpy(msg.data(), &len, 4);
    memcpy(msg.data() + 4, m_serverN.data(), m_serverKeyBytes);
    memcpy(msg.data() + 4 + m_serverKeyBytes, m_serverE.data(), m_serverKeyBytes);
    if (!RawSendAll(msg.data(), (DWORD)msg.size()))
        return Fail("failed to send server public key");
    return true;
}

bool RSAAESAuth::ReadClientPublicKey()
{
    BYTE hdr[4];
    if (!RawReadExact(hdr, 4))
        return Fail("failed to read client key length");
    m_clientKeyBits = Swap32IfLE(*(DWORD*)hdr);
    if (m_clientKeyBits < MinRsaKeyLength)
        return Fail("client RSA key too small");
    if (m_clientKeyBits > MaxRsaKeyLength)
        return Fail("client RSA key too big");
    m_clientKeyBytes = (m_clientKeyBits + 7) / 8;

    m_clientN.resize(m_clientKeyBytes);
    m_clientE.resize(m_clientKeyBytes);
    if (!RawReadExact(m_clientN.data(), m_clientKeyBytes)
        || !RawReadExact(m_clientE.data(), m_clientKeyBytes))
        return Fail("failed to read client public key");

    // Exponent must fit in 4 bytes (checked like the viewer does).
    for (DWORD i = 0; i < m_clientKeyBytes - 4; i++)
        if (m_clientE[i] != 0)
            return Fail("client RSA exponent too big");

    // Import the client key into CAPI for encrypting the server random.
    struct PubKeyBlob
    {
        BLOBHEADER  hdr;
        RSAPUBKEY   key;
        BYTE        modulus[MaxRsaKeyBytes];
    } keyBlob = { 0 };

    if (!m_hProv && !CryptAcquireContext(&m_hProv, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT))
        return Fail("CryptAcquireContext failed", GetLastError());
    keyBlob.hdr.bType = PUBLICKEYBLOB;
    keyBlob.hdr.bVersion = CUR_BLOB_VERSION;
    keyBlob.hdr.aiKeyAlg = CALG_RSA_KEYX;
    keyBlob.key.magic = BCRYPT_RSAPUBLIC_MAGIC; // RSA1
    keyBlob.key.bitlen = m_clientKeyBits;
    keyBlob.key.pubexp = Swap32IfLE(*(DWORD*)&m_clientE[m_clientKeyBytes - 4]);
    memcpy(keyBlob.modulus, m_clientN.data(), m_clientKeyBytes);
    ArraySwap(keyBlob.modulus, m_clientKeyBytes);
    if (!CryptImportKey(m_hProv, (BYTE*)&keyBlob,
                        offsetof(PubKeyBlob, modulus) + m_clientKeyBytes, 0, 0, &m_hClientKey))
        return Fail("invalid client RSA key", GetLastError());
    return true;
}

bool RSAAESAuth::SendServerRandom()
{
    BYTE buffer[MaxRsaKeyBytes];
    DWORD size = m_keySize / 8;

    if (!m_hProv && !CryptAcquireContext(&m_hProv, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT))
        return Fail("CryptAcquireContext failed", GetLastError());
    if (!CryptGenRandom(m_hProv, size, (BYTE*)m_serverRandom))
        return Fail("CryptGenRandom failed", GetLastError());
    memcpy(buffer, m_serverRandom, size);
    if (!CryptEncrypt(m_hClientKey, NULL, TRUE, 0, buffer, &size, sizeof(buffer)))
        return Fail("server random encryption failed", GetLastError());
    if (size != m_clientKeyBytes)
        return Fail("client key size mismatch");
    DWORD len16 = Swap16IfLE(size);
    ArraySwap(buffer, size); // CAPI emits little-endian ciphertext
    if (!RawSendAll((BYTE*)&len16, 2) || !RawSendAll(buffer, size))
        return Fail("failed to send server random");
    return true;
}

bool RSAAESAuth::ReadClientRandom()
{
    BYTE hdr[2];
    if (!RawReadExact(hdr, 2))
        return Fail("failed to read client random length");
    DWORD size = Swap16IfLE(*(WORD*)hdr);
    if (size != m_serverKeyBytes)
        return Fail("server key size does not match random length");

    std::vector<BYTE> ct(size);
    if (!RawReadExact(ct.data(), size))
        return Fail("failed to read client random");

    if (m_hServerKey)
    {
        // CNG: ciphertext is big-endian on the wire, no swap needed.
        DWORD plainLen = sizeof(m_clientRandom);
        SECURITY_STATUS st = NCryptDecrypt(m_hServerKey, ct.data(), size, NULL,
                                           m_clientRandom, plainLen, &plainLen, NCRYPT_PAD_PKCS1);
        if (st != ERROR_SUCCESS)
            return Fail("client random decryption failed", (DWORD)st);
        if (plainLen != m_keySize / 8)
            return Fail("client random size mismatch");
    }
    else
    {
        // Legacy CAPI key: acquire the AT_KEYEXCHANGE key from the provider
        // handle returned by CryptAcquireCertificatePrivateKey; ciphertext
        // must be little-endian.
        ArraySwap(ct.data(), size);
        HCRYPTKEY hPrivKey = 0;
        if (!CryptGetUserKey((HCRYPTPROV)m_hCertKey, AT_KEYEXCHANGE, &hPrivKey))
            return Fail("cannot access certificate private key", GetLastError());
        DWORD plainLen = size;
        BOOL ok = CryptDecrypt(hPrivKey, NULL, TRUE, 0, ct.data(), &plainLen);
        CryptDestroyKey(hPrivKey);
        if (!ok)
            return Fail("client random decryption failed", GetLastError());
        if (plainLen != m_keySize / 8)
            return Fail("client random size mismatch");
        memcpy(m_clientRandom, ct.data(), plainLen);
    }
    return true;
}

// Derive the EAX session keys and install the encrypted stream.
bool RSAAESAuth::SetCipher()
{
    ALG_ID algId = (m_keySize == 128 ? CALG_SHA1 : CALG_SHA_256);
    DWORD size = m_keySize / 8;
    BYTE readKey[32], writeKey[32];

    // Server reads with H(serverRand||clientRand), writes with
    // H(clientRand||serverRand) — mirrors the viewer's enc/dec derivation.
    auto Hash = [&](const BYTE* a, const BYTE* b, BYTE* out) -> bool {
        if (!m_hHash && !CryptCreateHash(m_hProv, algId, 0, 0, &m_hHash))
            return false;
        if (!CryptHashData(m_hHash, (BYTE*)a, size, 0) || !CryptHashData(m_hHash, (BYTE*)b, size, 0))
            return false;
        BYTE buf[256];
        DWORD bufSize = sizeof(buf);
        if (!CryptGetHashParam(m_hHash, HP_HASHVAL, buf, &bufSize, 0))
            return false;
        CryptDestroyHash(m_hHash);
        m_hHash = 0;
        memcpy(out, buf, size);
        return true;
    };

    if (!Hash(m_serverRandom, m_clientRandom, readKey)
        || !Hash(m_clientRandom, m_serverRandom, writeKey))
        return Fail("session key derivation failed");

    m_eax = new EAXStream();
    if (!m_eax->Init(m_keySize, writeKey, readKey))
        return Fail(m_eax->err);
    memset(readKey, 0, sizeof(readKey));
    memset(writeKey, 0, sizeof(writeKey));

    // Install the EAX layer on the socket for all subsequent traffic.
    auto callbacks = std::make_unique<VSocket::TLSCallbacks>();
    EAXStream* eax = m_eax;
    VSocket* socket = m_socket;

    callbacks->send = [eax, socket](const char* buf, int len) -> bool {
        std::vector<BYTE> frame;
        if (!eax->MakeMessage(buf, (DWORD)len, frame))
            return false;
        DWORD sent = 0;
        while (sent < (DWORD)frame.size())
        {
            int n = socket->Send((const char*)frame.data() + sent, (VCard)(frame.size() - sent));
            if (n <= 0)
                return false;
            sent += (DWORD)n;
        }
        return true;
    };
    callbacks->recv = [eax, socket](char* buf, int len, int* got) -> bool {
        *got = 0;
        if (eax->plain.empty())
        {
            // Read next EAX frame: u16be length + ciphertext + tag.
            BYTE hdr[2];
            DWORD got2 = 0;
            while (got2 < 2)
            {
                int n = socket->Read((char*)hdr + got2, 2 - (int)got2);
                if (n <= 0)
                    return false;
                got2 += (DWORD)n;
            }
            DWORD size = ((DWORD)hdr[0] << 8) | hdr[1];
            std::vector<BYTE> frame(EAXStream::AadSize + size + EAXStream::MacSize);
            frame[0] = hdr[0];
            frame[1] = hdr[1];
            DWORD off = EAXStream::AadSize;
            while (off < frame.size())
            {
                int n = socket->Read((char*)frame.data() + off, (int)(frame.size() - off));
                if (n <= 0)
                    return false;
                off += (DWORD)n;
            }
            if (!eax->OpenMessage(frame.data() + EAXStream::AadSize, size, eax->plain))
                return false;
        }
        DWORD take = min((DWORD)len, (DWORD)eax->plain.size());
        memcpy(buf, eax->plain.data(), take);
        eax->plain.erase(eax->plain.begin(), eax->plain.begin() + take);
        *got = (int)take;
        return true;
    };

    m_callbacks = std::move(callbacks);
    m_socket->SetTLSCallbacks(m_callbacks.get());
    m_eaxInstalled = true;
    return true;
}

// Encrypted helpers: one EAX message out / one EAX message in.

static bool EaxSend(RSAAESAuth* self, VSocket* socket, const void* data, DWORD len)
{
    (void)self;
    // The callbacks are installed; SendExact routes through them.
    return socket->SendExact((const char*)data, len) != FALSE;
}

bool RSAAESAuth::SendKeyHash()
{
    // Server hash = H(lenS|sN|sE|lenC|cN|cE)
    ALG_ID algId = (m_keySize == 128 ? CALG_SHA1 : CALG_SHA_256);
    DWORD hashSize = (m_keySize == 128 ? 20 : 32);
    BYTE hash[32];

    if (!m_hHash && !CryptCreateHash(m_hProv, algId, 0, 0, &m_hHash))
        return Fail("CryptCreateHash failed", GetLastError());
    DWORD sl = Swap32IfLE(m_serverKeyBits);
    DWORD cl = Swap32IfLE(m_clientKeyBits);
    if (!CryptHashData(m_hHash, (BYTE*)&sl, 4, 0)
        || !CryptHashData(m_hHash, m_serverN.data(), m_serverKeyBytes, 0)
        || !CryptHashData(m_hHash, m_serverE.data(), m_serverKeyBytes, 0)
        || !CryptHashData(m_hHash, (BYTE*)&cl, 4, 0)
        || !CryptHashData(m_hHash, m_clientN.data(), m_clientKeyBytes, 0)
        || !CryptHashData(m_hHash, m_clientE.data(), m_clientKeyBytes, 0))
        return Fail("key hash failed", GetLastError());
    BYTE buf[256];
    DWORD bufSize = sizeof(buf);
    if (!CryptGetHashParam(m_hHash, HP_HASHVAL, buf, &bufSize, 0))
        return Fail("key hash finalize failed", GetLastError());
    CryptDestroyHash(m_hHash);
    m_hHash = 0;
    memcpy(hash, buf, hashSize);

    if (!EaxSend(this, m_socket, hash, hashSize))
        return Fail("failed to send key hash");
    return true;
}

bool RSAAESAuth::ReadClientHash()
{
    DWORD hashSize = (m_keySize == 128 ? 20 : 32);
    std::vector<BYTE> recvHash(hashSize);
    if (!m_socket->ReadExact((char*)recvHash.data(), (VCard)hashSize))
        return Fail("failed to read client hash");

    // Expected client hash = H(lenC|cN|cE|lenS|sN|sE)
    ALG_ID algId = (m_keySize == 128 ? CALG_SHA1 : CALG_SHA_256);
    if (!m_hHash && !CryptCreateHash(m_hProv, algId, 0, 0, &m_hHash))
        return Fail("CryptCreateHash failed", GetLastError());
    DWORD cl = Swap32IfLE(m_clientKeyBits);
    DWORD sl = Swap32IfLE(m_serverKeyBits);
    if (!CryptHashData(m_hHash, (BYTE*)&cl, 4, 0)
        || !CryptHashData(m_hHash, m_clientN.data(), m_clientKeyBytes, 0)
        || !CryptHashData(m_hHash, m_clientE.data(), m_clientKeyBytes, 0)
        || !CryptHashData(m_hHash, (BYTE*)&sl, 4, 0)
        || !CryptHashData(m_hHash, m_serverN.data(), m_serverKeyBytes, 0)
        || !CryptHashData(m_hHash, m_serverE.data(), m_serverKeyBytes, 0))
        return Fail("client hash compute failed", GetLastError());
    BYTE buf[256];
    DWORD bufSize = sizeof(buf);
    if (!CryptGetHashParam(m_hHash, HP_HASHVAL, buf, &bufSize, 0))
        return Fail("client hash finalize failed", GetLastError());
    CryptDestroyHash(m_hHash);
    m_hHash = 0;

    if (!ArrayEqual(recvHash.data(), buf, hashSize))
        return Fail("client hash does not match");
    return true;
}

bool RSAAESAuth::SendSubtype()
{
    BYTE subtype = (BYTE)(m_requireUser ? secTypeRA2UserPass : secTypeRA2Pass);
    if (!EaxSend(this, m_socket, &subtype, 1))
        return Fail("failed to send subtype");
    return true;
}

bool RSAAESAuth::ReadCredentials(std::string& user, std::string& pass)
{
    BYTE ulen = 0, plen = 0;
    if (!m_socket->ReadExact((char*)&ulen, 1))
        return Fail("failed to read username length");
    std::vector<char> ubuf(ulen + 1);
    if (ulen && !m_socket->ReadExact(ubuf.data(), ulen))
        return Fail("failed to read username");
    if (!m_socket->ReadExact((char*)&plen, 1))
        return Fail("failed to read password length");
    std::vector<char> pbuf(plen + 1);
    if (plen && !m_socket->ReadExact(pbuf.data(), plen))
        return Fail("failed to read password");

    user = ubuf.data();
    pass = pbuf.data();
    memset(ubuf.data(), 0, ubuf.size());
    memset(pbuf.data(), 0, pbuf.size());
    return true;
}
