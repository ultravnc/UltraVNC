/////////////////////////////////////////////////////////////////////////////
//  Copyright (C) 2002-2024 UltraVNC Team Members. All Rights Reserved.
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
//  USA.
//
// If the source code for the program is not available from the place from
// which you received this file, check 
// http://www.uvnc.com/
//
////////////////////////////////////////////////////////////////////////////

// VeNCrypt authentication with TLS encrypted transport

#include "ClientConnection.h"
#include "Exception.h"
#include "AuthDialog.h"
#include "DSMPlugin/DSMPlugin.h"
#include "rdr/FdInStream.h"

#define SECURITY_WIN32
#include <security.h>
#include <Winternl.h>
#define SCHANNEL_USE_BLACKLISTS
#include <schannel.h>

#pragma comment(lib, "secur32.lib")
#pragma comment(lib, "crypt32.lib")

static char	lastError[1024];

struct DynBuffer
{
	static const int CapacityStep = 1024;

	BYTE		*buffer;
	DWORD		pos, size, capacity;

	DynBuffer() : buffer(0), pos(0), size(0), capacity(0) { }
	~DynBuffer()
	{
		delete [] buffer;
	}

	BYTE *EnsureFree(DWORD avail)
	{
		if (pos > 0 && pos == size)
			pos = size = 0;
		if (pos > 0 && size + avail > capacity)
		{
			size -= pos;
			memcpy(buffer, buffer + pos, size);
			pos = 0;
		}
		if (size + avail > capacity)
		{
			capacity = (size + avail + CapacityStep - 1) & -CapacityStep;
			BYTE *prev = buffer;
			buffer = new BYTE[capacity];
			memcpy(buffer, prev, size);
			delete [] prev;
		}
		return buffer + size;
	}

	inline BYTE *GetHead()
	{
		return buffer + pos;
	}

    inline BYTE *GetTail()
	{
		return buffer + size;
	}

	inline int GetAvailable()
	{
		return max(size - pos, 0);
	}

    inline int GetFree()
	{
		return max(capacity - size, 0);
	}
};

struct TLSTransport
{
    enum State
    {
        New = 0,
        Closed = 1,
        HandshakeStart = 2,
        PostHandshake = 8,
        Shutdown = 9,
    };

    char            *host;
    bool            noCertErrors;
    bool            isServer;
    PCCERT_CONTEXT  pCert;

    CredHandle      hCredentials;
    CtxtHandle      hContext;
    SecBufferDesc   inDesc, outDesc;
    SecBuffer       inBuffers[5], outBuffers[5];
    DWORD           contextReq;
    SecPkgContext_StreamSizes tlsSizes;
    PCERT_CONTEXT   pRemoteCertContext;
    State           state;

    TLSTransport() : isServer(false), host(NULL), pCert(NULL), contextReq(0), pRemoteCertContext(NULL), state(New)
    { 
        SecInvalidateHandle(&hCredentials);
        SecInvalidateHandle(&hContext);
        inDesc.ulVersion = SECBUFFER_VERSION;
        inDesc.cBuffers = _countof(inBuffers);
        inDesc.pBuffers = inBuffers;
        memset(inBuffers, 0, sizeof(inBuffers));
        outDesc.ulVersion = SECBUFFER_VERSION;
        outDesc.cBuffers = _countof(outBuffers);
        outDesc.pBuffers = outBuffers;
        memset(outBuffers, 0, sizeof(outBuffers));
    }

    TLSTransport(TLSTransport& from)
    {
        memcpy(this, &from, sizeof(TLSTransport));
        inDesc.pBuffers = inBuffers;
        outDesc.pBuffers = outBuffers;
        SecInvalidateHandle(&from.hContext);
        SecInvalidateHandle(&from.hCredentials);
        from.pRemoteCertContext = NULL;
    }

    ~TLSTransport()
    {
        if (SecIsValidHandle(&hContext))
            DeleteSecurityContext(&hContext);
        if (SecIsValidHandle(&hCredentials))
            FreeCredentialsHandle(&hCredentials);
        if (pRemoteCertContext)
            CertFreeCertificateContext(pRemoteCertContext);
    }

    void Init(char* _host, bool _noCertErrors, bool _isServer = false, PCCERT_CONTEXT _pCert = NULL)
    {
        host = _host;
        noCertErrors = _noCertErrors;
        isServer = _isServer;
        pCert = _pCert;
        state = HandshakeStart;
    }

    bool IsReady() { return state >= PostHandshake; }

    bool Handshake(DynBuffer &inbuf, DynBuffer &outbuf)
    {
        HRESULT hr;
        DWORD contextAttr;

        if (state == Closed)
            return SetLastError("Connection closed");
        if (contextReq == 0)
        {
            contextReq = ISC_REQ_REPLAY_DETECT | ISC_REQ_SEQUENCE_DETECT | ISC_REQ_CONFIDENTIALITY
                | ISC_REQ_ALLOCATE_MEMORY | ISC_REQ_EXTENDED_ERROR | ISC_REQ_STREAM;
        }
        if (!SecIsValidHandle(&hCredentials))
        {
            SCHANNEL_CRED cred = { 0 };
            cred.dwVersion = SCHANNEL_CRED_VERSION;
            cred.dwFlags = (noCertErrors ? SCH_CRED_MANUAL_CRED_VALIDATION | SCH_CRED_NO_DEFAULT_CREDS : 0) | SCH_CRED_REVOCATION_CHECK_CHAIN_EXCLUDE_ROOT;
            if (pCert)
            {
                cred.cCreds = 1;
                cred.paCred = &pCert;
                contextReq |= ISC_REQ_USE_SUPPLIED_CREDS;
            }
            if (TRUE) // Win10+ only
            {
                SCH_CREDENTIALS newCred = { 0 };
                TLS_PARAMETERS params = { 0 };
                newCred.dwVersion = SCH_CREDENTIALS_VERSION;
                newCred.cCreds = cred.cCreds;
                newCred.paCred = cred.paCred;
                newCred.dwFlags = cred.dwFlags | SCH_USE_STRONG_CRYPTO;
                newCred.cTlsParameters = 1;
                newCred.pTlsParameters = &params;
                if (cred.grbitEnabledProtocols)
                    params.grbitDisabledProtocols = ~cred.grbitEnabledProtocols;
                else
                    params.grbitDisabledProtocols = SP_PROT_TLS1_3;
                hr = AcquireCredentialsHandle(NULL, UNISP_NAME, (isServer ? SECPKG_CRED_INBOUND : SECPKG_CRED_OUTBOUND), NULL, &newCred, NULL, NULL, &hCredentials, NULL);
            }
            else hr = -1;
            if (FAILED(hr))
                hr = AcquireCredentialsHandle(NULL, UNISP_NAME, (isServer ? SECPKG_CRED_INBOUND : SECPKG_CRED_OUTBOUND), NULL, &cred, NULL, NULL, &hCredentials, NULL);
            if (FAILED(hr))
                return SetLastError("AcquireCredentialsHandle failed", hr);
        }
        bool done = false;
        while (!done)
        {
            PSecBufferDesc pIn = NULL;
            if (inbuf.size > 0)
            {
                inBuffers[0].BufferType = SECBUFFER_TOKEN;
                inBuffers[0].pvBuffer = inbuf.GetHead();
                inBuffers[0].cbBuffer = inbuf.GetAvailable();
                pIn = &inDesc;
            }
            if (isServer)
            {
                hr = AcceptSecurityContext(&hCredentials, SecIsValidHandle(&hContext) ? &hContext : NULL, pIn, contextReq,
                    SECURITY_NATIVE_DREP, &hContext, &outDesc, &contextAttr, NULL);
            }
            else
            {
                hr = InitializeSecurityContext(&hCredentials, SecIsValidHandle(&hContext) ? &hContext : NULL, host, contextReq, 0,
                    SECURITY_NATIVE_DREP, pIn, 0, &hContext, &outDesc, &contextAttr, NULL);
            }
            if (hr == SEC_E_INCOMPLETE_MESSAGE)
            {
                memset(inBuffers, 0, sizeof(inBuffers));
                break;
            }
            inbuf.size = 0;
            if (FAILED(hr))
                return SetLastError("InitializeSecurityContext failed", hr);
            for (DWORD i = 0; i < inDesc.cBuffers; i++)
            {
                if (inBuffers[i].cbBuffer > 0 && inBuffers[i].BufferType == SECBUFFER_EXTRA)
                {
                    BYTE* dst = inbuf.EnsureFree(inBuffers[i].cbBuffer);
                    memcpy(dst, inBuffers[i].pvBuffer, inBuffers[i].cbBuffer);
                    inbuf.size += inBuffers[i].cbBuffer;
                }
            }
            memset(inBuffers, 0, sizeof(inBuffers));
            for (DWORD i = 0; i < outDesc.cBuffers; i++)
            {
                if (outBuffers[i].cbBuffer > 0)
                {
                    if (outBuffers[i].BufferType == SECBUFFER_TOKEN)
                    {
                        BYTE* dst = outbuf.EnsureFree(outBuffers[i].cbBuffer);
                        memcpy(dst, outBuffers[i].pvBuffer, outBuffers[i].cbBuffer);
                        outbuf.size += outBuffers[i].cbBuffer;
                    }
                    FreeContextBuffer(outBuffers[i].pvBuffer);
                }
            }
            memset(outBuffers, 0, sizeof(outBuffers));
            switch (hr)
            {
            case SEC_I_CONTINUE_NEEDED:
                // do nothing
                break;
            case SEC_E_OK:
                hr = QueryContextAttributes(&hContext, SECPKG_ATTR_STREAM_SIZES, &tlsSizes);
                if (FAILED(hr))
                    return SetLastError("QueryContextAttributes failed", hr);
                if(tlsSizes.cBuffers > _countof(inBuffers) || tlsSizes.cBuffers > _countof(outBuffers))
                    return SetLastError("Not enough I/O buffers");
                inDesc.cBuffers = tlsSizes.cBuffers;
                outDesc.cBuffers = tlsSizes.cBuffers;
                if (pRemoteCertContext)
                    CertFreeCertificateContext(pRemoteCertContext), pRemoteCertContext = NULL;
                hr = QueryContextAttributes(&hContext, SECPKG_ATTR_REMOTE_CERT_CONTEXT, &pRemoteCertContext);
                if (FAILED(hr))
                    pRemoteCertContext = NULL;
                state = PostHandshake;
                done = true;
                {
                    SecPkgContext_CipherInfo cipherInfo;
                    SecPkgContext_ConnectionInfo connInfo;
                    if (QueryContextAttributes(&hContext, SECPKG_ATTR_CIPHER_INFO, &cipherInfo) == S_OK)
                        vnclog.Print(0, _T("TLSTransport: Using %ls (0x%X) ciphersuite\n"), cipherInfo.szCipherSuite, cipherInfo.dwCipherSuite);
                    if (QueryContextAttributes(&hContext, SECPKG_ATTR_CONNECTION_INFO, &connInfo) == S_OK)
                        vnclog.Print(0, _T("TLSTransport: %s using %s cipher with %s hash and %s key-exchange\n"), GetAlgName(connInfo.dwProtocol), 
                            GetAlgName(connInfo.aiCipher), GetAlgName(connInfo.aiHash), GetAlgName(connInfo.aiExch));
                }
                break;
            case SEC_I_CONTEXT_EXPIRED:
                state = Shutdown;
                done = true;
                break;
            default:
                return SetLastError("Handshake cannot be completed", hr);
            }
            if (inbuf.size == 0)
                break;
        }
        return true;
    }

    bool Send(BYTE *pDataBuffer, int nDataLen, DynBuffer &outbuf)
    {
        if (state == Closed || !SecIsValidHandle(&hContext))
            return SetLastError("Connection closed");
        int cnt = (nDataLen + tlsSizes.cbMaximumMessage - 1) / tlsSizes.cbMaximumMessage;
        outbuf.EnsureFree(tlsSizes.cbHeader * cnt + nDataLen + tlsSizes.cbTrailer * cnt);
        for (int i = 0; i < nDataLen; i += tlsSizes.cbMaximumMessage)
        {
            BYTE *dst = outbuf.EnsureFree(tlsSizes.cbHeader + nDataLen + tlsSizes.cbTrailer);
            outBuffers[0].BufferType = SECBUFFER_STREAM_HEADER;
            outBuffers[0].pvBuffer = dst;
            outBuffers[0].cbBuffer = tlsSizes.cbHeader;
            dst += tlsSizes.cbHeader;
            outBuffers[1].BufferType = SECBUFFER_DATA;
            outBuffers[1].pvBuffer = dst;
            outBuffers[1].cbBuffer = nDataLen;
            memcpy(dst, pDataBuffer, nDataLen);
            dst += nDataLen;
            outBuffers[2].BufferType = SECBUFFER_STREAM_TRAILER;
            outBuffers[2].pvBuffer = dst;
            outBuffers[2].cbBuffer = tlsSizes.cbTrailer;
            for (DWORD i = 3; i < inDesc.cBuffers; i++)
                outBuffers[i].BufferType = SECBUFFER_EMPTY;
            HRESULT hr = EncryptMessage(&hContext, 0, &outDesc, 0);
            if (FAILED(hr))
                return SetLastError("EncryptMessage failed", hr);
            // use cbBuffer sizes as returned by EncryptMessage in buffers, not original sizes from tlsSizes
            outbuf.size += outBuffers[0].cbBuffer + outBuffers[1].cbBuffer + outBuffers[2].cbBuffer;
            memset(outBuffers, 0, sizeof(outBuffers));
        }
        return true;
    }

    bool Receive(DynBuffer &inbuf, DynBuffer &plainbuf, DynBuffer &outbuf)
    {
        if (state == Closed || !SecIsValidHandle(&hContext))
            return SetLastError("Connection closed");
        bool done = false;
        while (!done && inbuf.size > 0)
        {
            inBuffers[0].BufferType = SECBUFFER_DATA;
            inBuffers[0].pvBuffer = inbuf.GetHead();
            inBuffers[0].cbBuffer = inbuf.size;
            HRESULT hr = DecryptMessage(&hContext, &inDesc, 0, NULL);
            if (hr == SEC_E_INCOMPLETE_MESSAGE)
            {
                memset(inBuffers, 0, sizeof(inBuffers));
                return false;
            }
            if (hr == SEC_E_INVALID_HANDLE) // session on hContext already closed
                break;
            if (FAILED(hr))
                return SetLastError("DecryptMessage failed", hr);
            DWORD origsize = inbuf.size;
            inbuf.size = 0;
            for (DWORD i = 0; i < inDesc.cBuffers; i++)
            {
                if (inBuffers[i].cbBuffer > 0)
                {
                    if (inBuffers[i].BufferType == SECBUFFER_DATA)
                    {
                        BYTE *dst = plainbuf.EnsureFree(inBuffers[i].cbBuffer);
                        memcpy(dst, inBuffers[i].pvBuffer, inBuffers[i].cbBuffer);
                        plainbuf.size += inBuffers[i].cbBuffer;
                    }
                    else if (inBuffers[i].BufferType == SECBUFFER_EXTRA)
                    {
                        BYTE *dst = inbuf.EnsureFree(inBuffers[i].cbBuffer);
                        void *src = inBuffers[i].pvBuffer;
                        if (!src) 
                            src = inbuf.buffer + inBuffers[0].cbBuffer - inBuffers[i].cbBuffer;
                        memcpy(dst, src, inBuffers[i].cbBuffer);
                        inbuf.size += inBuffers[i].cbBuffer;
                    }
                }
            }
            memset(inBuffers, 0, sizeof(inBuffers));
            switch (hr)
            {
            case SEC_E_OK:
                break;
            case SEC_I_RENEGOTIATE:
                state = HandshakeStart;
                if (!Handshake(DynBuffer(), outbuf))
                    return false;
                done = true;
                break;
            case SEC_I_CONTEXT_EXPIRED:
                state = Shutdown;
                done = true;
                break;
            }
        }
        return true;
    }

private:
    static char *GetAlgName(ALG_ID algId)
    {
        switch (algId)
        {
        case 0x8:       return "SSL2_CLIENT";
        case 0x20:      return "SSL3_CLIENT";
        case 0x80:      return "TLS1_0_CLIENT";
        case 0x200:     return "TLS1_1_CLIENT";
        case 0x800:     return "TLS1_2_CLIENT";
        case 0x2000:    return "TLS1_3_CLIENT";
        case 0x4:       return "SSL2_SERVER";
        case 0x10:      return "SSL3_SERVER";
        case 0x40:      return "TLS1_0_SERVER";
        case 0x100:     return "TLS1_1_SERVER";
        case 0x400:     return "TLS1_2_SERVER";
        case 0x1000:    return "TLS1_3_SERVER";
        case 0x6602:    return "RC2";
        case 0x6801:    return "RC4";
        case 0x6601:    return "DES";
        case 0x6603:    return "3DES";
        case 0x660E:    return "AES_128";
        case 0x660F:    return "AES_192";
        case 0x6610:    return "AES_256";
        case 0x8001:    return "MD2";
        case 0x8003:    return "MD5";
        case 0x8004:    return "SHA1";
        case 0x800C:    return "SHA_256";
        case 0x800D:    return "SHA_384";
        case 0x800E:    return "SHA_512";
        case 0xA400:    return "RSA_KEYX";
        case 0x2400:    return "RSA_SIGN";
        case 0xAA02:    return "DH_EPHEM";
        case 0xAA05:    return "ECDH";
        case 0xAE06:    return "ECDH_EPHEM";
        default:
            sprintf_s(lastError, "0x%X", algId);
            return lastError;
        }
    }

	static bool SetLastError(char *error, HRESULT hr = S_OK)
	{
        strcpy_s(lastError, error);
        if (FAILED(hr))
        {
            char msg[1024];
            FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, hr, 0, msg, _countof(msg), NULL);
            sprintf_s(lastError, "%s. %s", error, msg);
        }
	    vnclog.Print(0, FAILED(hr) ? _T("TLSTransport: %s (0x%X)\n") : _T("TLSTransport: %s\n"), lastError, hr);
		throw WarningException(lastError);
		return false;
	}
};

struct TLSPlugin : public IPlugin
{
    TLSTransport    chan;
    DynBuffer	    encBuffer, decBuffer, decPlain;
    int             wanted;

    TLSPlugin(TLSTransport &c) : chan(c) { }

    virtual ~TLSPlugin() { }


    virtual BYTE *TransformBuffer(BYTE *pDataBuffer, int nDataLen, int *pnTransformedDataLen)
    {
        chan.Send(pDataBuffer, nDataLen, encBuffer);
        *pnTransformedDataLen = encBuffer.size;
        encBuffer.size = 0;
        return encBuffer.buffer;
    }

    virtual BYTE *RestoreBuffer(BYTE *pTransBuffer, int nTransDataLen, int *pnRestoredDataLen)
    {
        if (!pTransBuffer)
        {
            wanted = nTransDataLen;
            if (decPlain.GetAvailable() >= wanted)
            {
                *pnRestoredDataLen = 0;
                return decBuffer.buffer + decBuffer.size;
            }
            else
                *pnRestoredDataLen = 1;
			BYTE *dst = decBuffer.EnsureFree(*pnRestoredDataLen);
			decBuffer.size += *pnRestoredDataLen;
			return dst;
        }
        if (decBuffer.GetAvailable() > 0)
        {
            DynBuffer outBuffer;
            chan.Receive(decBuffer, decPlain, outBuffer);
            if (outBuffer.size > 0)
                throw WarningException("TLSPlugin: Renegotiation requested");
        }
        if (decPlain.GetAvailable() >= wanted)
		{
			*pnRestoredDataLen = wanted;
			memcpy(pTransBuffer, decPlain.GetHead(), wanted);
			decPlain.pos += wanted;
		}
		else
			*pnRestoredDataLen = -1;
        return NULL;
    }
};

const int secTypeTLSNone = 257;
const int secTypeTLSVnc = 258;
const int secTypeTLSPlain = 259;
const int secTypeX509None = 260;
const int secTypeX509Vnc = 261;
const int secTypeX509Plain = 262;

void ClientConnection::AuthVeNCrypt()
{
    int version, temp, size, subType;

    ReadExact((char *)&version, 2);
    version = Swap16IfLE(version);
    if (version >= 2)
    {
        version = Swap16IfLE(2);
        WriteExact((char *)&version, 2);
    }
    else
    {
        version = 0;
        WriteExact((char*)&version, 2);
        throw WarningException("AuthVeNCrypt: Unsupported version");
    }
    temp = 0;
    ReadExact((char *)&temp, 1);
    if (temp)
        throw WarningException("AuthVeNCrypt: Server reported unsupported version");
    size = 0;
    ReadExact((char *)&size, 1);
    subType = -1;
    for (int i = 0; i < size; i++)
    {
        ReadExact((char *)&temp, 4);
        if (subType < 0)
        {
            temp = Swap32IfLE(temp);
            switch (temp)
            {
            case secTypeTLSNone:
            case secTypeTLSPlain:
            case secTypeX509None:
            case secTypeX509Plain:
                subType = temp;
                break;
            }
        }
    }
    if (subType < 0)
        throw WarningException("AuthVeNCrypt: No valid sub-type");
    temp = Swap32IfLE(subType);
    WriteExact((char *)&temp, 4);
    temp = 0;
    ReadExact((char *)&temp, 1);
    if (temp != 1)
        throw WarningException("AuthVeNCrypt: Server unsupported sub-type");
    TLSTransport chan;
    chan.Init(m_host, !(subType >= secTypeX509None));
    DynBuffer inbuf, outbuf;
    while (TRUE)
    {
        if (!chan.Handshake(inbuf, outbuf))
            return;
        if (chan.IsReady())
            break;
        size = outbuf.GetAvailable();
        if (size > 0)
        {
            WriteExact((char *)outbuf.GetHead(), size);
            outbuf.size = 0;
        }
        size = 1; // fis->Check_if_buffer_has_data();
        inbuf.EnsureFree(size);
        ReadExact((char*)inbuf.GetTail(), size);
        inbuf.size += size;
    }
    m_fUsePlugin = true;
    m_pPluginInterface = new TLSPlugin(chan);
    if (subType == secTypeTLSPlain || subType == secTypeX509Plain)
    {
        if (strlen(m_clearPasswd) == 0)
        {
            AuthDialog ad;
            if (!ad.DoDialog(false, m_host, m_port, true))
                throw QuietException("Authentication cancelled");
            strcpy_s(m_cmdlnUser, ad.m_user);
            strcpy_s(m_clearPasswd, ad.m_passwd);
        }
        temp = (int)strlen(m_cmdlnUser);
        temp = Swap32IfLE(temp);
        WriteExact((char *)&temp, 4);
        temp = (int)strlen(m_clearPasswd);
        temp = Swap32IfLE(temp);
        WriteExact((char *)&temp, 4);
        WriteExact(m_cmdlnUser, (int)strlen(m_cmdlnUser));
        WriteExact(m_clearPasswd, (int)strlen(m_clearPasswd));
    }
}
