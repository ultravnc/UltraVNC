// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
//

#include "stdhdrs.h"
#include "SchannelTLS.h"
#include "vsocket.h"
#include "vnclog.h"

extern VNCLog vnclog;

// IO_BUFFER_SIZE for raw token exchange during handshake
static const DWORD IO_BUFFER_SIZE = 32768;

// -----------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------

static std::string HrStr(SECURITY_STATUS hr)
{
    char buf[32];
    sprintf_s(buf, "0x%08X", (unsigned)hr);
    return buf;
}

// Convert hex thumbprint string (40 chars) to 20-byte binary
static bool HexToBin(const char* hex, BYTE* out, DWORD outLen)
{
    DWORD len = (DWORD)strlen(hex);
    if (len != outLen * 2) return false;
    for (DWORD i = 0; i < outLen; i++)
    {
        char hi = hex[i * 2];
        char lo = hex[i * 2 + 1];
        auto nib = [](char c) -> int {
            if (c >= '0' && c <= '9') return c - '0';
            if (c >= 'a' && c <= 'f') return c - 'a' + 10;
            if (c >= 'A' && c <= 'F') return c - 'A' + 10;
            return -1;
        };
        int h = nib(hi), l = nib(lo);
        if (h < 0 || l < 0) return false;
        out[i] = (BYTE)((h << 4) | l);
    }
    return true;
}

// -----------------------------------------------------------------------
// SchannelTLS
// -----------------------------------------------------------------------

SchannelTLS::SchannelTLS()
    : m_certCtx(nullptr)
    , m_credValid(false)
    , m_ctxValid(false)
    , m_ready(false)
{
    SecInvalidateHandle(&m_credHandle);
    SecInvalidateHandle(&m_ctxHandle);
    memset(&m_streamSizes, 0, sizeof(m_streamSizes));
}

SchannelTLS::~SchannelTLS()
{
    if (m_ctxValid)  { DeleteSecurityContext(&m_ctxHandle);    m_ctxValid  = false; }
    if (m_credValid) { FreeCredentialsHandle(&m_credHandle);   m_credValid = false; }
    if (m_certCtx)   { CertFreeCertificateContext(m_certCtx);  m_certCtx   = nullptr; }
}

// -----------------------------------------------------------------------
// Certificate loading
// -----------------------------------------------------------------------

static PCCERT_CONTEXT FindCertInStore(HCERTSTORE hStore, const BYTE* sha1, DWORD sha1Len)
{
    CRYPT_HASH_BLOB blob;
    blob.cbData = sha1Len;
    blob.pbData = const_cast<BYTE*>(sha1);
    return CertFindCertificateInStore(hStore, X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                                      0, CERT_FIND_SHA1_HASH, &blob, nullptr);
}

bool SchannelTLS::LoadCertByThumbprint(const char* thumbprint)
{
    if (!thumbprint || strlen(thumbprint) != 40)
    {
        vnclog.Print(LL_CONNERR, VNCLOG("TLS: invalid thumbprint (must be 40 hex chars)\n"));
        return false;
    }

    BYTE sha1[20];
    if (!HexToBin(thumbprint, sha1, 20))
    {
        vnclog.Print(LL_CONNERR, VNCLOG("TLS: thumbprint is not valid hex\n"));
        return false;
    }

    // Try Local Machine first, then Current User
    const DWORD locations[] = { CERT_SYSTEM_STORE_LOCAL_MACHINE, CERT_SYSTEM_STORE_CURRENT_USER };
    for (DWORD loc : locations)
    {
        HCERTSTORE hStore = CertOpenStore(CERT_STORE_PROV_SYSTEM, 0, 0,
                                          loc | CERT_STORE_READONLY_FLAG, L"MY");
        if (!hStore) continue;
        PCCERT_CONTEXT ctx = FindCertInStore(hStore, sha1, 20);
        CertCloseStore(hStore, 0);
        if (ctx)
        {
            if (m_certCtx) CertFreeCertificateContext(m_certCtx);
            m_certCtx = ctx;

            // Log subject
            char subject[256] = {};
            CertGetNameStringA(m_certCtx, CERT_NAME_SIMPLE_DISPLAY_TYPE, 0, nullptr, subject, sizeof(subject));
            vnclog.Print(LL_CLIENTS, VNCLOG("TLS: loaded cert subject='%s'\n"), subject);

            // Verify not expired
            FILETIME ftNow;
            GetSystemTimeAsFileTime(&ftNow);
            if (CompareFileTime(&m_certCtx->pCertInfo->NotAfter, &ftNow) < 0)
            {
                vnclog.Print(LL_CONNERR, VNCLOG("TLS: certificate expired\n"));
                CertFreeCertificateContext(m_certCtx);
                m_certCtx = nullptr;
                return false;
            }

            // Verify private key accessible
            HCRYPTPROV_OR_NCRYPT_KEY_HANDLE hKey = 0;
            DWORD keySpec = 0;
            BOOL mustFree = FALSE;
            if (!CryptAcquireCertificatePrivateKey(m_certCtx,
                    CRYPT_ACQUIRE_SILENT_FLAG | CRYPT_ACQUIRE_PREFER_NCRYPT_KEY_FLAG,
                    nullptr, &hKey, &keySpec, &mustFree))
            {
                vnclog.Print(LL_CONNERR, VNCLOG("TLS: private key not accessible: %lu\n"), GetLastError());
                CertFreeCertificateContext(m_certCtx);
                m_certCtx = nullptr;
                return false;
            }
            if (mustFree)
            {
                if (keySpec == CERT_NCRYPT_KEY_SPEC)
                    NCryptFreeObject(hKey);
                else
                    CryptReleaseContext(hKey, 0);
            }

            vnclog.Print(LL_CLIENTS, VNCLOG("TLS: certificate ready\n"));
            return true;
        }
    }

    vnclog.Print(LL_CONNERR, VNCLOG("TLS: certificate not found in store\n"));
    return false;
}

bool SchannelTLS::EnsureSelfSignedCert(char* outThumbprint)
{
    const wchar_t* CN = L"CN=UltraVNC-WebViewer";

    // Try Local Machine store first (requires admin/SYSTEM), fall back to Current User
    struct StoreCandidate { DWORD location; bool machineKeyset; const wchar_t* label; };
    StoreCandidate candidates[] = {
        { CERT_SYSTEM_STORE_LOCAL_MACHINE, true,  L"LocalMachine" },
        { CERT_SYSTEM_STORE_CURRENT_USER,  false, L"CurrentUser"  },
    };

    HCERTSTORE hStore = nullptr;
    bool useMachineKeyset = false;
    for (auto& c : candidates)
    {
        hStore = CertOpenStore(CERT_STORE_PROV_SYSTEM, 0, 0, c.location, L"MY");
        if (hStore)
        {
            useMachineKeyset = c.machineKeyset;
            vnclog.Print(LL_CLIENTS, VNCLOG("TLS: using cert store %S\\MY\n"), c.label);
            break;
        }
        vnclog.Print(LL_INTINFO, VNCLOG("TLS: cannot open %S store: %lu, trying next\n"), c.label, GetLastError());
    }

    if (!hStore)
    {
        vnclog.Print(LL_CONNERR, VNCLOG("TLS: cannot open any cert store\n"));
        return false;
    }

    // Search for existing valid self-signed cert
    PCCERT_CONTEXT existing = nullptr;
    PCCERT_CONTEXT cur = nullptr;
    while ((cur = CertEnumCertificatesInStore(hStore, cur)) != nullptr)
    {
        char name[256] = {};
        CertGetNameStringA(cur, CERT_NAME_SIMPLE_DISPLAY_TYPE, 0, nullptr, name, sizeof(name));
        if (strcmp(name, "UltraVNC-WebViewer") == 0)
        {
            FILETIME ftNow;
            GetSystemTimeAsFileTime(&ftNow);
            if (CompareFileTime(&cur->pCertInfo->NotAfter, &ftNow) > 0)
            {
                existing = CertDuplicateCertificateContext(cur);
                CertFreeCertificateContext(cur);
                break;
            }
        }
    }

    if (existing)
    {
        if (m_certCtx) CertFreeCertificateContext(m_certCtx);
        m_certCtx = existing;
        CertCloseStore(hStore, 0);
        vnclog.Print(LL_CLIENTS, VNCLOG("TLS: reusing existing self-signed certificate\n"));
    }
    else
    {
        // Encode subject name
        CERT_NAME_BLOB nameBlob = {};
        DWORD nameLen = 0;
        if (!CertStrToNameW(X509_ASN_ENCODING, CN, CERT_X500_NAME_STR, nullptr, nullptr, &nameLen, nullptr))
        {
            CertCloseStore(hStore, 0);
            return false;
        }
        std::vector<BYTE> nameBuf(nameLen);
        CertStrToNameW(X509_ASN_ENCODING, CN, CERT_X500_NAME_STR, nullptr, nameBuf.data(), &nameLen, nullptr);
        nameBlob.cbData = nameLen;
        nameBlob.pbData = nameBuf.data();

        // 2-year validity
        SYSTEMTIME start, end;
        GetSystemTime(&start);
        end = start;
        end.wYear += 2;

        CRYPT_KEY_PROV_INFO keyProvInfo = {};
        keyProvInfo.pwszContainerName = const_cast<LPWSTR>(L"UltraVNC-WebViewer");
        keyProvInfo.dwProvType        = PROV_RSA_SCHANNEL;
        keyProvInfo.dwFlags           = useMachineKeyset ? CRYPT_MACHINE_KEYSET : 0;
        keyProvInfo.dwKeySpec         = AT_KEYEXCHANGE;

        CRYPT_ALGORITHM_IDENTIFIER sigAlg = {};
        sigAlg.pszObjId = const_cast<LPSTR>(szOID_RSA_SHA256RSA);

        PCCERT_CONTEXT newCtx = CertCreateSelfSignCertificate(
            (HCRYPTPROV_OR_NCRYPT_KEY_HANDLE)0, &nameBlob,
            0, &keyProvInfo, &sigAlg,
            &start, &end, nullptr);

        if (!newCtx)
        {
            vnclog.Print(LL_CONNERR, VNCLOG("TLS: self-signed cert creation failed: %lu\n"), GetLastError());
            CertCloseStore(hStore, 0);
            return false;
        }

        // Add to store
        if (!CertAddCertificateContextToStore(hStore, newCtx,
                CERT_STORE_ADD_REPLACE_EXISTING, &m_certCtx))
        {
            vnclog.Print(LL_CONNERR, VNCLOG("TLS: cannot add cert to store: %lu\n"), GetLastError());
            CertFreeCertificateContext(newCtx);
            CertCloseStore(hStore, 0);
            return false;
        }
        CertFreeCertificateContext(newCtx);
        CertCloseStore(hStore, 0);
        vnclog.Print(LL_CLIENTS, VNCLOG("TLS: created new self-signed certificate\n"));
    }

    // Extract thumbprint and convert to hex string
    BYTE sha1[20];
    DWORD sha1Len = sizeof(sha1);
    if (!CertGetCertificateContextProperty(m_certCtx, CERT_SHA1_HASH_PROP_ID, sha1, &sha1Len))
    {
        vnclog.Print(LL_CONNERR, VNCLOG("TLS: cannot get cert thumbprint\n"));
        return false;
    }
    if (outThumbprint)
    {
        for (int i = 0; i < 20; i++)
            sprintf_s(outThumbprint + i * 2, 3, "%02x", sha1[i]);
        outThumbprint[40] = '\0';
        vnclog.Print(LL_CLIENTS, VNCLOG("TLS: self-signed cert thumbprint: %s\n"), outThumbprint);
    }

    return true;
}

// -----------------------------------------------------------------------
// Handshake
// -----------------------------------------------------------------------

bool SchannelTLS::ServerHandshake(VSocket* sock)
{
    if (!m_certCtx)
    {
        vnclog.Print(LL_CONNERR, VNCLOG("TLS: no certificate loaded\n"));
        return false;
    }

    // Build SCHANNEL_CRED
    SCHANNEL_CRED sc = {};
    sc.dwVersion       = SCHANNEL_CRED_VERSION;
    sc.cCreds          = 1;
    sc.paCred          = &m_certCtx;
#ifndef SP_PROT_TLS1_3_SERVER
#define SP_PROT_TLS1_3_SERVER 0x00001000
#endif
    sc.grbitEnabledProtocols = SP_PROT_TLS1_2_SERVER | SP_PROT_TLS1_3_SERVER;
    sc.dwFlags         = SCH_CRED_NO_SYSTEM_MAPPER | SCH_USE_STRONG_CRYPTO;
    sc.dwMinimumCipherStrength = 128;

    // Acquire credentials
    TimeStamp tsExpiry;
    SECURITY_STATUS ss = AcquireCredentialsHandleW(
        nullptr, UNISP_NAME_W,
        SECPKG_CRED_INBOUND, nullptr, &sc,
        nullptr, nullptr, &m_credHandle, &tsExpiry);

    if (ss != SEC_E_OK)
    {
        vnclog.Print(LL_CONNERR, VNCLOG("TLS: AcquireCredentialsHandle failed: %s\n"), HrStr(ss).c_str());
        return false;
    }
    m_credValid = true;

    // Handshake loop
    SecBuffer extraData = {};
    if (!HandshakeLoop(sock, true, &extraData))
        return false;

    // If server sent extra data after handshake, buffer it
    if (extraData.cbBuffer > 0 && extraData.pvBuffer)
    {
        m_rawBuf.insert(m_rawBuf.end(),
            (BYTE*)extraData.pvBuffer,
            (BYTE*)extraData.pvBuffer + extraData.cbBuffer);
        FreeContextBuffer(extraData.pvBuffer);
    }

    // Get stream sizes for encrypt/decrypt
    ss = QueryContextAttributes(&m_ctxHandle, SECPKG_ATTR_STREAM_SIZES, &m_streamSizes);
    if (ss != SEC_E_OK)
    {
        vnclog.Print(LL_CONNERR, VNCLOG("TLS: QueryContextAttributes failed: %s\n"), HrStr(ss).c_str());
        return false;
    }

    m_ready = true;
    vnclog.Print(LL_CLIENTS, VNCLOG("TLS: handshake complete\n"));
    return true;
}

bool SchannelTLS::HandshakeLoop(VSocket* sock, bool firstCall, SecBuffer* pExtraData)
{
    std::vector<BYTE> ioBuffer(IO_BUFFER_SIZE);
    DWORD ioBufferUsed = 0;
    SECURITY_STATUS ss = SEC_I_CONTINUE_NEEDED;

    while (ss == SEC_I_CONTINUE_NEEDED || ss == SEC_E_INCOMPLETE_MESSAGE || firstCall)
    {
        firstCall = false;

        // Read data from client if needed
        if (ioBufferUsed == 0 || ss == SEC_E_INCOMPLETE_MESSAGE)
        {
            if (ioBufferUsed == ioBuffer.size())
            {
                vnclog.Print(LL_CONNERR, VNCLOG("TLS: handshake buffer overflow\n"));
                return false;
            }
            DWORD received = 0;
            if (!RawRecv(sock, ioBuffer.data() + ioBufferUsed,
                         (DWORD)(ioBuffer.size() - ioBufferUsed), &received))
            {
                vnclog.Print(LL_CONNERR, VNCLOG("TLS: handshake recv failed\n"));
                return false;
            }
            ioBufferUsed += received;
        }

        // Input buffer: what we just read
        SecBuffer inBufs[2];
        inBufs[0].BufferType = SECBUFFER_TOKEN;
        inBufs[0].cbBuffer   = ioBufferUsed;
        inBufs[0].pvBuffer   = ioBuffer.data();
        inBufs[1].BufferType = SECBUFFER_EMPTY;
        inBufs[1].cbBuffer   = 0;
        inBufs[1].pvBuffer   = nullptr;

        SecBufferDesc inDesc;
        inDesc.ulVersion = SECBUFFER_VERSION;
        inDesc.cBuffers  = 2;
        inDesc.pBuffers  = inBufs;

        // Output buffer: token to send to client
        SecBuffer outBufs[1];
        outBufs[0].BufferType = SECBUFFER_TOKEN;
        outBufs[0].cbBuffer   = 0;
        outBufs[0].pvBuffer   = nullptr;

        SecBufferDesc outDesc;
        outDesc.ulVersion = SECBUFFER_VERSION;
        outDesc.cBuffers  = 1;
        outDesc.pBuffers  = outBufs;

        DWORD ctxAttr = 0;
        TimeStamp tsExpiry;

        ss = AcceptSecurityContext(
            &m_credHandle,
            m_ctxValid ? &m_ctxHandle : nullptr,
            &inDesc,
            ASC_REQ_SEQUENCE_DETECT | ASC_REQ_REPLAY_DETECT |
            ASC_REQ_CONFIDENTIALITY | ASC_REQ_EXTENDED_ERROR |
            ASC_REQ_ALLOCATE_MEMORY | ASC_REQ_STREAM,
            SECURITY_NATIVE_DREP,
            &m_ctxHandle,
            &outDesc,
            &ctxAttr,
            &tsExpiry);

        m_ctxValid = true;

        // Send any token back to client
        if (outBufs[0].cbBuffer > 0 && outBufs[0].pvBuffer)
        {
            if (!RawSend(sock, (BYTE*)outBufs[0].pvBuffer, outBufs[0].cbBuffer))
            {
                FreeContextBuffer(outBufs[0].pvBuffer);
                vnclog.Print(LL_CONNERR, VNCLOG("TLS: handshake send failed\n"));
                return false;
            }
            FreeContextBuffer(outBufs[0].pvBuffer);
        }

        if (ss == SEC_E_OK)
        {
            // Check for extra data (app data that came after handshake)
            if (inBufs[1].BufferType == SECBUFFER_EXTRA && inBufs[1].cbBuffer > 0)
            {
                pExtraData->pvBuffer   = LocalAlloc(LMEM_FIXED, inBufs[1].cbBuffer);
                pExtraData->cbBuffer   = inBufs[1].cbBuffer;
                pExtraData->BufferType = SECBUFFER_TOKEN;
                if (pExtraData->pvBuffer)
                    memcpy(pExtraData->pvBuffer,
                           ioBuffer.data() + ioBufferUsed - inBufs[1].cbBuffer,
                           inBufs[1].cbBuffer);
            }
            else
            {
                pExtraData->pvBuffer   = nullptr;
                pExtraData->cbBuffer   = 0;
                pExtraData->BufferType = SECBUFFER_EMPTY;
            }
            return true;
        }
        else if (ss == SEC_I_CONTINUE_NEEDED)
        {
            // Consume processed bytes; keep EXTRA bytes if any
            if (inBufs[1].BufferType == SECBUFFER_EXTRA && inBufs[1].cbBuffer > 0)
            {
                DWORD extra = inBufs[1].cbBuffer;
                memmove(ioBuffer.data(), ioBuffer.data() + ioBufferUsed - extra, extra);
                ioBufferUsed = extra;
            }
            else
            {
                ioBufferUsed = 0;
            }
        }
        else if (ss == SEC_E_INCOMPLETE_MESSAGE)
        {
            // Need more data — loop again without clearing ioBuffer
        }
        else
        {
            vnclog.Print(LL_CONNERR, VNCLOG("TLS: AcceptSecurityContext failed: %s\n"), HrStr(ss).c_str());
            return false;
        }
    }

    return (ss == SEC_E_OK);
}

// -----------------------------------------------------------------------
// Send / Recv
// -----------------------------------------------------------------------

bool SchannelTLS::Send(VSocket* sock, const char* buf, int len)
{
    if (!m_ready) return false;

    int offset = 0;
    while (offset < len)
    {
        // Max plaintext per record
        int chunk = min(len - offset, (int)m_streamSizes.cbMaximumMessage);

        std::vector<BYTE> msg(m_streamSizes.cbHeader + chunk + m_streamSizes.cbTrailer);

        SecBuffer bufs[4];
        bufs[0].BufferType = SECBUFFER_STREAM_HEADER;
        bufs[0].cbBuffer   = m_streamSizes.cbHeader;
        bufs[0].pvBuffer   = msg.data();

        bufs[1].BufferType = SECBUFFER_DATA;
        bufs[1].cbBuffer   = (ULONG)chunk;
        bufs[1].pvBuffer   = msg.data() + m_streamSizes.cbHeader;
        memcpy(bufs[1].pvBuffer, buf + offset, chunk);

        bufs[2].BufferType = SECBUFFER_STREAM_TRAILER;
        bufs[2].cbBuffer   = m_streamSizes.cbTrailer;
        bufs[2].pvBuffer   = msg.data() + m_streamSizes.cbHeader + chunk;

        bufs[3].BufferType = SECBUFFER_EMPTY;
        bufs[3].cbBuffer   = 0;
        bufs[3].pvBuffer   = nullptr;

        SecBufferDesc desc;
        desc.ulVersion = SECBUFFER_VERSION;
        desc.cBuffers  = 4;
        desc.pBuffers  = bufs;

        SECURITY_STATUS ss = EncryptMessage(&m_ctxHandle, 0, &desc, 0);
        if (ss != SEC_E_OK)
        {
            vnclog.Print(LL_CONNERR, VNCLOG("TLS: EncryptMessage failed: %s\n"), HrStr(ss).c_str());
            return false;
        }

        DWORD totalEncrypted = bufs[0].cbBuffer + bufs[1].cbBuffer + bufs[2].cbBuffer;
        if (!RawSend(sock, msg.data(), totalEncrypted))
            return false;

        offset += chunk;
    }
    return true;
}

bool SchannelTLS::Recv(VSocket* sock, char* buf, int len, int* received)
{
    *received = 0;
    if (!m_ready) return false;

    // Drain any already-decrypted plaintext first
    if (!m_plainBuf.empty())
    {
        int take = min(len, (int)m_plainBuf.size());
        memcpy(buf, m_plainBuf.data(), take);
        m_plainBuf.erase(m_plainBuf.begin(), m_plainBuf.begin() + take);
        *received = take;
        return true;
    }

    // Read raw encrypted bytes and decrypt
    for (;;)
    {
        // Try to decrypt what we have in m_rawBuf
        if (!m_rawBuf.empty())
        {
            SecBuffer bufs[4];
            bufs[0].BufferType = SECBUFFER_DATA;
            bufs[0].cbBuffer   = (ULONG)m_rawBuf.size();
            bufs[0].pvBuffer   = m_rawBuf.data();
            bufs[1].BufferType = SECBUFFER_EMPTY;
            bufs[2].BufferType = SECBUFFER_EMPTY;
            bufs[3].BufferType = SECBUFFER_EMPTY;
            for (int i = 1; i < 4; i++) { bufs[i].cbBuffer = 0; bufs[i].pvBuffer = nullptr; }

            SecBufferDesc desc;
            desc.ulVersion = SECBUFFER_VERSION;
            desc.cBuffers  = 4;
            desc.pBuffers  = bufs;

            SECURITY_STATUS ss = DecryptMessage(&m_ctxHandle, &desc, 0, nullptr);

            if (ss == SEC_E_OK)
            {
                // Find data buffer
                BYTE* plain = nullptr;
                DWORD plainLen = 0;
                BYTE* extra = nullptr;
                DWORD extraLen = 0;

                for (int i = 1; i < 4; i++)
                {
                    if (bufs[i].BufferType == SECBUFFER_DATA)
                    {
                        plain    = (BYTE*)bufs[i].pvBuffer;
                        plainLen = bufs[i].cbBuffer;
                    }
                    else if (bufs[i].BufferType == SECBUFFER_EXTRA)
                    {
                        extra    = (BYTE*)bufs[i].pvBuffer;
                        extraLen = bufs[i].cbBuffer;
                    }
                }

                // Remaining undecrypted bytes
                if (extraLen > 0 && extra)
                    m_rawBuf.assign(extra, extra + extraLen);
                else
                    m_rawBuf.clear();

                // Return plaintext to caller
                if (plain && plainLen > 0)
                {
                    int take = min(len, (int)plainLen);
                    memcpy(buf, plain, take);
                    *received = take;
                    // Buffer the rest
                    if ((DWORD)take < plainLen)
                        m_plainBuf.insert(m_plainBuf.end(), plain + take, plain + plainLen);
                    return true;
                }
                // else: decrypted to nothing (heartbeat etc.) — loop
                continue;
            }
            else if (ss == SEC_I_CONTEXT_EXPIRED)
            {
                // Peer sent close_notify
                return false;
            }
            else if (ss == SEC_E_INCOMPLETE_MESSAGE)
            {
                // Need more data — fall through to read more
            }
            else
            {
                vnclog.Print(LL_CONNERR, VNCLOG("TLS: DecryptMessage failed: %s\n"), HrStr(ss).c_str());
                return false;
            }
        }

        // Read more raw bytes from socket
        BYTE tmp[4096];
        DWORD got = 0;
        if (!RawRecv(sock, tmp, sizeof(tmp), &got) || got == 0)
            return false;
        m_rawBuf.insert(m_rawBuf.end(), tmp, tmp + got);
    }
}

void SchannelTLS::Shutdown(VSocket* sock)
{
    if (!m_ctxValid) return;

    DWORD shutType = SCHANNEL_SHUTDOWN;
    SecBuffer shutBuf;
    shutBuf.cbBuffer   = sizeof(shutType);
    shutBuf.BufferType = SECBUFFER_TOKEN;
    shutBuf.pvBuffer   = &shutType;

    SecBufferDesc shutDesc;
    shutDesc.ulVersion = SECBUFFER_VERSION;
    shutDesc.cBuffers  = 1;
    shutDesc.pBuffers  = &shutBuf;

    ApplyControlToken(&m_ctxHandle, &shutDesc);

    // Build close_notify record
    SecBuffer outBuf;
    outBuf.cbBuffer   = 0;
    outBuf.BufferType = SECBUFFER_TOKEN;
    outBuf.pvBuffer   = nullptr;

    SecBufferDesc outDesc;
    outDesc.ulVersion = SECBUFFER_VERSION;
    outDesc.cBuffers  = 1;
    outDesc.pBuffers  = &outBuf;

    DWORD ctxAttr = 0;
    TimeStamp tsExpiry;
    AcceptSecurityContext(&m_credHandle, &m_ctxHandle, nullptr,
        ASC_REQ_SEQUENCE_DETECT | ASC_REQ_REPLAY_DETECT |
        ASC_REQ_CONFIDENTIALITY | ASC_REQ_EXTENDED_ERROR |
        ASC_REQ_ALLOCATE_MEMORY | ASC_REQ_STREAM,
        SECURITY_NATIVE_DREP, &m_ctxHandle, &outDesc, &ctxAttr, &tsExpiry);

    if (outBuf.pvBuffer && outBuf.cbBuffer)
    {
        RawSend(sock, (BYTE*)outBuf.pvBuffer, outBuf.cbBuffer);
        FreeContextBuffer(outBuf.pvBuffer);
    }

    m_ready = false;
}

// -----------------------------------------------------------------------
// Raw socket I/O (bypasses VSocket's HTTP wrappers to use the SOCKET directly)
// -----------------------------------------------------------------------

bool SchannelTLS::RawSend(VSocket* sock, const BYTE* data, DWORD len)
{
    DWORD sent = 0;
    while (sent < len)
    {
        int n = sock->Send((const char*)data + sent, (VCard)(len - sent));
        if (n <= 0)
            return false;
        sent += (DWORD)n;
    }
    return true;
}

bool SchannelTLS::RawRecv(VSocket* sock, BYTE* data, DWORD len, DWORD* received)
{
    // We need at least 1 byte; use non-exact read to avoid blocking on full len
    int n = sock->Read((char*)data, (int)len);
    if (n <= 0)
    {
        *received = 0;
        return false;
    }
    *received = (DWORD)n;
    return true;
}
