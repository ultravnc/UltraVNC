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
//  If the source code for the program is not available from the place from
//  which you received this file, check
//  https://uvnc.com/
//
////////////////////////////////////////////////////////////////////////////


// VeNCrypt authentication with TLS encrypted transport

#include "ClientConnection.h"
#include "Exception.h"
#include "AuthDialog.h"

#define SECURITY_WIN32
#include <security.h>
#include <winternl.h>
#define SCHANNEL_USE_BLACKLISTS
#include <schannel.h>
#include <cryptuiapi.h>
#include <commctrl.h>
#include <wincrypt.h>
#include <shlwapi.h>

#pragma comment(lib, "secur32.lib")
#pragma comment(lib, "crypt32.lib")
#pragma comment(lib, "cryptui.lib")

// Dynamicly sized I/O buffers

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

	inline BYTE *GetHead() { return buffer + pos; }

	inline BYTE *GetTail() { return buffer + size; }

	inline int GetAvailable() { return max(size - pos, 0); }

	inline int GetFree() { return max(capacity - size, 0); }
};

// Schannel based TLS wrapper implemented as a "Sans I/O" protocol

struct TLSSession
{
	enum State
	{
		StateNew = 0,
		StateClosed = 1,
		StateHandshakeStart = 2,
		StatePostHandshake = 8,
		StateShutdown = 9,
	};

	State			state;
	char			*hostName;
	bool			isServer;
	PCCERT_CONTEXT	pLocalCert, pRemoteCert;
	DWORD			contextReq;
	CredHandle		hCredentials;
	CtxtHandle		hContext;
	SecBufferDesc	inDesc, outDesc;
	SecBuffer		inBuffers[5], outBuffers[5];
	SecPkgContext_StreamSizes tlsSizes;
	char			lastError[1024];

	TLSSession() : state(StateNew), hostName(NULL), isServer(false), pLocalCert(NULL), pRemoteCert(NULL), contextReq(0)
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

	TLSSession(TLSSession& from)
	{
		memcpy(this, &from, sizeof(TLSSession));
		inDesc.pBuffers = inBuffers;
		outDesc.pBuffers = outBuffers;
		SecInvalidateHandle(&from.hContext);
		SecInvalidateHandle(&from.hCredentials);
		from.pLocalCert = from.pRemoteCert = NULL;
	}

	~TLSSession()
	{
		if (SecIsValidHandle(&hContext))
			DeleteSecurityContext(&hContext);
		if (SecIsValidHandle(&hCredentials))
			FreeCredentialsHandle(&hCredentials);
		if (pLocalCert)
			CertFreeCertificateContext(pLocalCert);
		if (pRemoteCert)
			CertFreeCertificateContext(pRemoteCert);
	}

	void Init(char *_hostName, bool _isServer = false, PCERT_CONTEXT _pLocalCert = NULL)
	{
		hostName = _hostName;
		isServer = _isServer;
		pLocalCert = _pLocalCert;
		state = StateHandshakeStart;
	}

	bool IsReady() { return state >= StatePostHandshake; }

	bool Handshake(DynBuffer &inbuf, DynBuffer &outbuf)
	{
		HRESULT hr;
		DWORD contextAttr;

		if (state == StateClosed)
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
			cred.dwFlags = SCH_CRED_MANUAL_CRED_VALIDATION | SCH_CRED_NO_DEFAULT_CREDS | SCH_CRED_REVOCATION_CHECK_CHAIN_EXCLUDE_ROOT;
			if (pLocalCert)
			{
				cred.cCreds = 1;
				cred.paCred = &pLocalCert;
				contextReq |= ISC_REQ_USE_SUPPLIED_CREDS;
			}
			if (TRUE) // Win11+ only
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
				hr = AcquireCredentialsHandle(NULL, UNISP_NAME, (isServer ? SECPKG_CRED_INBOUND : SECPKG_CRED_OUTBOUND), NULL,
					&newCred, NULL, NULL, &hCredentials, NULL);
			}
			else hr = -1;
			if (FAILED(hr))
				hr = AcquireCredentialsHandle(NULL, UNISP_NAME, (isServer ? SECPKG_CRED_INBOUND : SECPKG_CRED_OUTBOUND), NULL,
					&cred, NULL, NULL, &hCredentials, NULL);
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
				hr = AcceptSecurityContext(&hCredentials, SecIsValidHandle(&hContext) ? &hContext : NULL, pIn, contextReq,
					SECURITY_NATIVE_DREP, &hContext, &outDesc, &contextAttr, NULL);
			else
				hr = InitializeSecurityContext(&hCredentials, SecIsValidHandle(&hContext) ? &hContext : NULL, hostName, contextReq, 0,
					SECURITY_NATIVE_DREP, pIn, 0, &hContext, &outDesc, &contextAttr, NULL);
			if (hr == SEC_E_INCOMPLETE_MESSAGE)
			{
				memset(inBuffers, 0, sizeof(inBuffers));
				break;
			}
			inbuf.size = 0;
			if (FAILED(hr))
				return SetLastError(isServer ? "AcceptSecurityContext failed" : "InitializeSecurityContext failed", hr);
			for (DWORD i = 0; i < inDesc.cBuffers; i++)
			{
				if (inBuffers[i].cbBuffer > 0 && inBuffers[i].BufferType == SECBUFFER_EXTRA)
				{
					BYTE *dst = inbuf.EnsureFree(inBuffers[i].cbBuffer);
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
						BYTE *dst = outbuf.EnsureFree(outBuffers[i].cbBuffer);
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
				QueryContextAttributes(&hContext, SECPKG_ATTR_REMOTE_CERT_CONTEXT, &pRemoteCert);
				state = StatePostHandshake;
				done = true;
				{
					SecPkgContext_CipherInfo cipherInfo;
					SecPkgContext_ConnectionInfo connInfo;
					if (QueryContextAttributes(&hContext, SECPKG_ATTR_CIPHER_INFO, &cipherInfo) == S_OK)
						vnclog.Print(0, _T("TLSSession: Using %ls (0x%X) ciphersuite\n"), cipherInfo.szCipherSuite, cipherInfo.dwCipherSuite);
					if (QueryContextAttributes(&hContext, SECPKG_ATTR_CONNECTION_INFO, &connInfo) == S_OK)
						vnclog.Print(0, _T("TLSSession: %s using %s cipher with %s hash and %s key-exchange\n"), GetAlgName(connInfo.dwProtocol), 
							GetAlgName(connInfo.aiCipher), GetAlgName(connInfo.aiHash), GetAlgName(connInfo.aiExch));
				}
				break;
			case SEC_I_CONTEXT_EXPIRED:
				state = StateShutdown;
				return SetLastError("Handshake aborted", hr);
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
		if (state == StateClosed || !SecIsValidHandle(&hContext))
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
		if (state == StateClosed || !SecIsValidHandle(&hContext))
			return SetLastError("Connection closed");
		bool done = false;
		while (!done && inbuf.size > 0)
		{
			inBuffers[0].BufferType = SECBUFFER_DATA;
			inBuffers[0].pvBuffer = inbuf.GetHead();
			inBuffers[0].cbBuffer = inbuf.GetAvailable();
			HRESULT hr = DecryptMessage(&hContext, &inDesc, 0, NULL);
			if (hr == SEC_E_INCOMPLETE_MESSAGE)
			{
				memset(inBuffers, 0, sizeof(inBuffers));
				return true;
			}
			if (hr == SEC_E_INVALID_HANDLE) // session on hContext already closed
				break;
			if (FAILED(hr))
				return SetLastError("DecryptMessage failed", hr);
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
				state = StateHandshakeStart;
				if (!Handshake(DynBuffer(), outbuf))
					return false;
				done = true;
				break;
			case SEC_I_CONTEXT_EXPIRED:
				state = StateShutdown;
				done = true;
				break;
			default:
				return SetLastError("Unexpected DecryptMessage result", hr);
			}
		}
		return true;
	}

	bool Shutdown(DynBuffer& outbuf)
	{
		HRESULT hr;
		DWORD contextAttr;
		DWORD type = SCHANNEL_SHUTDOWN;

		inBuffers[0].BufferType = SECBUFFER_TOKEN;
		inBuffers[0].pvBuffer = &type;
		inBuffers[0].cbBuffer = sizeof(type);
		// note: passing more than one input buffer fails w/ SEC_E_INVALID_TOKEN (&H80090308)
		inDesc.cBuffers = 1; 
		hr = ApplyControlToken(&hContext, &inDesc);
		inDesc.cBuffers = tlsSizes.cBuffers;
		if (FAILED(hr))
			return SetLastError("ApplyControlToken failed", hr);
		outBuffers[0].BufferType = SECBUFFER_TOKEN;
		if (isServer)
			hr = AcceptSecurityContext(&hCredentials, &hContext, NULL, contextReq,
					SECURITY_NATIVE_DREP, &hContext, &outDesc, &contextAttr, NULL);
		else
			hr = InitializeSecurityContext(&hCredentials, &hContext, hostName, contextReq, 0,
					SECURITY_NATIVE_DREP, NULL, 0, &hContext, &outDesc, &contextAttr, NULL);
		if (FAILED(hr))
			return SetLastError(isServer ? "AcceptSecurityContext failed" : "InitializeSecurityContext failed", hr);
		memset(inBuffers, 0, sizeof(inBuffers));
		for (DWORD i = 0; i < outDesc.cBuffers; i++)
		{
			if (outBuffers[i].cbBuffer > 0)
			{
				if (outBuffers[i].BufferType == SECBUFFER_TOKEN)
				{
					BYTE *dst = outbuf.EnsureFree(outBuffers[i].cbBuffer);
					memcpy(dst, outBuffers[i].pvBuffer, outBuffers[i].cbBuffer);
					outbuf.size += outBuffers[i].cbBuffer;
				}
				FreeContextBuffer(outBuffers[i].pvBuffer);
			}
		}
		memset(outBuffers, 0, sizeof(outBuffers));
		return true;
	}

	bool ValidateRemoteCertificate()
	{
		bool isvalid = false;
		std::wstring host = toWide(hostName);
		PCCERT_CHAIN_CONTEXT pChainContext = NULL;
		CERT_CHAIN_PARA chainParams = { 0 };
		CERT_CHAIN_POLICY_PARA policyPara = { 0 };
		CERT_CHAIN_POLICY_STATUS policyStatus = { 0 };
		HTTPSPolicyCallbackData httpsPolicy = { 0 };

		chainParams.cbSize = sizeof(chainParams);
		if (!CertGetCertificateChain(NULL, pRemoteCert, NULL, NULL, &chainParams, CERT_CHAIN_REVOCATION_CHECK_CHAIN, 0, &pChainContext))
		{
			SetLastError("CertGetCertificateChain failed", FACILITY_WIN32 | GetLastError());
			goto Exit;
		}
		if (pChainContext->cChain == 0)
		{
			SetLastError(NULL, CERT_E_CHAINING);
			goto Exit;
		}
		httpsPolicy.cbSize = sizeof(httpsPolicy);
		httpsPolicy.dwAuthType = (isServer ? AUTHTYPE_CLIENT : AUTHTYPE_SERVER);
		httpsPolicy.pwszServerName = (isServer ? NULL : (WCHAR *)host.c_str());
		policyPara.cbSize = sizeof(policyPara);
		policyPara.pvExtraPolicyPara = &httpsPolicy;
		policyStatus.cbSize = sizeof(policyStatus);
		if (!CertVerifyCertificateChainPolicy(CERT_CHAIN_POLICY_SSL, pChainContext, &policyPara, &policyStatus))
		{
			SetLastError("CertVerifyCertificateChainPolicy failed", FACILITY_WIN32 | GetLastError());
			goto Exit;
		}
		if (FAILED(policyStatus.dwError))
		{
			SetLastError(NULL, policyStatus.dwError);
			goto Exit;
		}
		isvalid = true;
	Exit:
		if (pChainContext)
			CertFreeCertificateChain(pChainContext);
		return isvalid;
	}

private:
	static std::wstring toWide(const char *src)
	{
		std::wstring dst(strlen(src), L' ');
		dst.resize(mbstowcs(&dst[0], src, strlen(src)));
		return dst;
	}

	char *GetAlgName(ALG_ID algId)
	{
		switch (algId)
		{
		case SP_PROT_SSL2_SERVER:	return "SSL2_SERVER";
		case SP_PROT_SSL2_CLIENT:	return "SSL2_CLIENT";
		case SP_PROT_SSL3_SERVER:	return "SSL3_SERVER";
		case SP_PROT_SSL3_CLIENT:	return "SSL3_CLIENT";
		case SP_PROT_TLS1_0_SERVER:	return "TLS1_0_SERVER";
		case SP_PROT_TLS1_0_CLIENT:	return "TLS1_0_CLIENT";
		case SP_PROT_TLS1_1_SERVER:	return "TLS1_1_SERVER";
		case SP_PROT_TLS1_1_CLIENT:	return "TLS1_1_CLIENT";
		case SP_PROT_TLS1_2_SERVER:	return "TLS1_2_SERVER";
		case SP_PROT_TLS1_2_CLIENT:	return "TLS1_2_CLIENT";
		case SP_PROT_TLS1_3_SERVER:	return "TLS1_3_SERVER";
		case SP_PROT_TLS1_3_CLIENT:	return "TLS1_3_CLIENT";
		case CALG_RC2:				return "RC2";
		case CALG_RC4:				return "RC4";
		case CALG_DES:				return "DES";
		case CALG_3DES:				return "3DES";
		case CALG_AES_128:			return "AES_128";
		case CALG_AES_192:			return "AES_192";
		case CALG_AES_256:			return "AES_256";
		case CALG_MD2:				return "MD2";
		case CALG_MD5:				return "MD5";
		case CALG_SHA1:				return "SHA1";
		case CALG_SHA_256:			return "SHA_256";
		case CALG_SHA_384:			return "SHA_384";
		case CALG_SHA_512:			return "SHA_512";
		case CALG_RSA_KEYX:			return "RSA_KEYX";
		case CALG_RSA_SIGN:			return "RSA_SIGN";
		case CALG_DH_EPHEM:			return "DH_EPHEM";
		case CALG_ECDH:				return "ECDH";
		case CALG_ECDH_EPHEM:		return "ECDH_EPHEM";
		default:
			sprintf_s(lastError, "0x%X", algId);
			return lastError;
		}
	}

	bool SetLastError(char *error, HRESULT hr = S_OK)
	{
		if (hr != S_OK)
		{
			char msg[1024];
			FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, hr, 0, msg, _countof(msg), NULL);
			for (int i = (int)strlen(msg) - 1; i >= 0 && !msg[i + 1]; i--)
				if (msg[i] == '\r' || msg[i] == '\n' || msg[i] == '.')
					msg[i] = '\0';
			if (error)
				sprintf_s(lastError, "%s. %s (0x%X)", error, msg, hr);
			else
				sprintf_s(lastError, "%s (0x%X)", msg, hr);
		} 
		else strcpy_s(lastError, error);
		return false;
	}
};

// Encryption/decryption plugin based on TLS session

struct TLSPlugin : public IPlugin
{
	TLSSession		session;
	DynBuffer	    encBuffer, decBuffer, decPlain;
	int             wanted;

	TLSPlugin(TLSSession &s) : session(s) { }

	virtual ~TLSPlugin() { }

	virtual BYTE *TransformBuffer(BYTE *pDataBuffer, int nDataLen, int *pnTransformedDataLen)
	{
		if (!session.Send(pDataBuffer, nDataLen, encBuffer))
			SetLastError(session.lastError);
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
				return decBuffer.buffer; // anything non-NULL
			}
			*pnRestoredDataLen = 1;
			BYTE *dst = decBuffer.EnsureFree(*pnRestoredDataLen);
			decBuffer.size += *pnRestoredDataLen;
			return dst;
		}
		if (decBuffer.GetAvailable() > 0)
		{
			DynBuffer outBuffer;
			if (!session.Receive(decBuffer, decPlain, outBuffer))
				SetLastError(session.lastError);
			if (outBuffer.size > 0)
				SetLastError("Renegotiation request not implemented");
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

private:
	static bool SetLastError(char *error)
	{
		vnclog.Print(0, _T("TLSPlugin: %s\n"), error);
		throw WarningException(error);
		return false;
	}
};

const int secTypeTLSNone = 257;
const int secTypeTLSVnc = 258;
const int secTypeTLSPlain = 259;
const int secTypeX509None = 260;
const int secTypeX509Vnc = 261;
const int secTypeX509Plain = 262;

static const TCHAR *ThumbprintSection = _T("thumbprint");

struct ThumbHost
{
	TCHAR *key, *fname;

	ThumbHost(TCHAR *k, TCHAR *f) : key(k), fname(f) { }

	bool CompareThumbprint(char *hex)
	{ 
		TCHAR buf[64] = { 0 };
		GetPrivateProfileString(ThumbprintSection, key, NULL, buf, sizeof(buf), fname);
		return _tcscmp(hex, buf) == 0;
	}
	
	void SaveThumbprint(char *hex) { WritePrivateProfileString(ThumbprintSection, key, hex, fname); }
};

static bool GetCertificateThumbprint(PCCERT_CONTEXT pCert, char *hex, int hexsize)
{
	BYTE thumb[64] = { 0 };
	DWORD size = sizeof(thumb);
	if (!pCert || !CertGetCertificateContextProperty(pCert, CERT_HASH_PROP_ID, thumb, &size))
		return false;
	snprintf(hex, hexsize, "%02x", thumb[0]);
	for (DWORD i = 1; i < size; i++)
		snprintf(hex + strlen(hex), hexsize - strlen(hex), "-%02x", thumb[i]);
	return true;
}

void ClientConnection::AuthVeNCrypt()
{
	int version, temp, size, subType;
	auto SetLastError = [](char *error)
	{
		vnclog.Print(0, _T("AuthVeNCrypt: %s\n"), error);
		throw WarningException(error);
	};

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
		WriteExact((char *)&version, 2);
		return SetLastError("Unsupported version");
	}
	temp = 0;
	ReadExact((char *)&temp, 1);
	if (temp)
		return SetLastError("Server reported unsupported version");
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
			case secTypeTLSVnc:
			case secTypeTLSPlain:
			case secTypeX509None:
			case secTypeX509Vnc:
			case secTypeX509Plain:
				subType = temp;
				break;
			}
		}
	}
	if (subType < 0)
		return SetLastError("No valid sub-type");
	temp = Swap32IfLE(subType);
	WriteExact((char *)&temp, 4);
	temp = 0;
	ReadExact((char *)&temp, 1);
	if (temp != 1)
		return SetLastError("Server unsupported sub-type");

	// start TLS on existing connection
	TLSSession session;
	session.Init(m_host);
	DynBuffer inbuf, outbuf;
	while (TRUE)
	{
		if (!session.Handshake(inbuf, outbuf))
			return SetLastError(session.lastError);
		size = outbuf.GetAvailable();
		if (size > 0)
		{
			WriteExact((char *)outbuf.GetHead(), size);
			outbuf.size = 0;
		}
		if (session.IsReady())
			break;
		size = 1;
		inbuf.EnsureFree(size);
		ReadExact((char *)inbuf.GetTail(), size);
		inbuf.size += size;
	}
	if (!session.ValidateRemoteCertificate())
	{
		TCHAR key[MAX_HOST_NAME_LEN];
		sprintf_s(key, "%s:%d", m_host, m_port);
		ThumbHost host(key, m_opts->getDefaultOptionsFileName());
		char hex[64];
		bool done = false;
		if (GetCertificateThumbprint(session.pRemoteCert, hex, sizeof(hex)) && host.CompareThumbprint(hex))
			done = true;
		while (!done)
		{
			int nButtonPressed = 0;
			WCHAR msg[1024];
			swprintf_s(msg, L"%hs\n\nDo you want to continue?\n", session.lastError);
			TASKDIALOGCONFIG tc = { 0 };
			const TASKDIALOG_BUTTON buttons[] = { { IDNO, L"View Certificate" } };
			BOOL bPersist = FALSE;
			tc.cbSize = sizeof(tc);
			tc.dwFlags = TDF_SIZE_TO_CONTENT;
			tc.dwCommonButtons = TDCBF_OK_BUTTON | TDCBF_CANCEL_BUTTON;
			tc.pszWindowTitle = L"UltraVNC Viewer - Warning";
			tc.hInstance = pApp->m_instance;
			tc.pszMainIcon = MAKEINTRESOURCEW(IDR_TRAY);
			tc.pszMainInstruction = L"Invalid server certificate";
			tc.pszContent = msg;
			tc.pButtons = buttons;
			tc.cButtons = _countof(buttons);
			tc.nDefaultButton = 1;
			if (secTypeTLSNone <= subType && subType <= secTypeTLSPlain)
				tc.pszVerificationText = L"Don't ask anymore";
			TaskDialogIndirect(&tc, &nButtonPressed, NULL, &bPersist);
			switch (nButtonPressed)
			{
			case IDNO:
			{
				BOOL changed = FALSE;
				CRYPTUI_VIEWCERTIFICATE_STRUCT vc = { 0 };
				vc.dwSize = sizeof(vc);
				vc.hwndParent = m_hwndStatus;
				vc.pCertContext = session.pRemoteCert;
				CryptUIDlgViewCertificate(&vc, &changed);
				break;
			}
			case IDCANCEL:
				if (!session.Shutdown(outbuf))
					return SetLastError(session.lastError);
				size = outbuf.GetAvailable();
				if (size > 0)
				{
					WriteExact((char*)outbuf.GetHead(), size);
					outbuf.size = 0;
				}
				throw QuietException("Authentication cancelled");
				break;
			default:
				if (bPersist)
					host.SaveThumbprint(hex);
				done = true;
				break;
			}
		}
	}
	m_fUsePlugin = true;
	m_pPluginInterface = new TLSPlugin(session);	
	switch (subType)
	{
	case secTypeTLSNone:
	case secTypeX509None:
		// do nothing
		break;
	case secTypeTLSVnc:
	case secTypeX509Vnc:
		AuthVnc();
		break;
	case secTypeTLSPlain:
	case secTypeX509Plain:
		if (strlen(m_clearPasswd) == 0)
		{
			AuthDialog ad;
			if (!ad.DoDialog(dtUserPass, m_host, m_port))
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
		break;
	default:
		return SetLastError("Cannot complete sub-type authentication");
	}
}
