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

// RSA-AES authentication and encryption

#include "ClientConnection.h"
#include "Exception.h"
#include "AuthDialog.h"
#include <commctrl.h>
extern "C" {
#include "../common/mnemonic.h"
}


const int secTypeRA2UserPass = 1;
const int secTypeRA2Pass = 2;

static char	lastError[1024];
char hex[24];
char catchphrase[1024];

static void ArraySwap(BYTE *p, DWORD size)
{
	for (DWORD i = 0; i < size / 2; i++)
	{
		const BYTE t = p[i];
		p[i] = p[size - 1 - i];
		p[size - 1 - i] = t;
	}
}

static bool ArrayEqual(BYTE *a, BYTE *b, DWORD size)
{
	BYTE r = 0;
	for (DWORD i = 0; i < size; i++)
		r |= a[i] ^ b[i];
	return r == 0;
}

// AES cipher implementation as wrapper for Crypto API forward permutaion

struct AESCipher
{
	static const DWORD BlockSize = 16;

	struct SymKeyBlob
	{
		BLOBHEADER  hdr;
		DWORD		cbKeySize;
		BYTE		key[32];
	};

	HCRYPTPROV	hProv;
	HCRYPTKEY	hKey;

	AESCipher() : hProv(0), hKey(0) { }

	~AESCipher()
	{
		if (hKey)
			CryptDestroyKey(hKey);
		if (hProv)
			CryptReleaseContext(hProv, 0);
	}

	bool Init(DWORD keySize, void *key)
	{
		if (!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT))
		{
			sprintf_s(lastError, "CryptAcquireContext failed (%d)", GetLastError());
			return SetLastError(lastError);
		}
		SymKeyBlob keyBlob = { 0 };
		keyBlob.hdr.bType = PLAINTEXTKEYBLOB;
		keyBlob.hdr.bVersion = CUR_BLOB_VERSION;
		keyBlob.hdr.aiKeyAlg = (keySize == 128 ? CALG_AES_128 : CALG_AES_256);
		keyBlob.cbKeySize = min(keySize / 8, sizeof(keyBlob.key));
		memcpy(keyBlob.key, key, keyBlob.cbKeySize);
		if (!CryptImportKey(hProv, (BYTE *)&keyBlob, offsetof(SymKeyBlob, key) + keyBlob.cbKeySize, NULL, 0, &hKey))
		{
			sprintf_s(lastError, "CryptImportKey failed (%d)", GetLastError());
			return SetLastError(lastError);
		}
		DWORD mode = CRYPT_MODE_ECB;
		if (!CryptSetKeyParam(hKey, KP_MODE, (BYTE *)&mode, 0))
		{
			sprintf_s(lastError, "CryptSetKeyParam(KP_MODE) failed (%d)", GetLastError());
			return SetLastError(lastError);
		}
		return true;
	}

	bool Encrypt(void *buf, DWORD size = BlockSize)
	{
		if (!CryptEncrypt(hKey, NULL, FALSE, 0, (BYTE *)buf, &size, size))
		{
			sprintf_s(lastError, "CryptEncrypt failed (%d)", GetLastError());
			return SetLastError(lastError);
		}
		return true;
	}

private:
	static bool SetLastError(char *error)
	{
		vnclog.Print(0, _T("AESCipher: %s\n"), error);
		throw WarningException(error);
		return false;
	}
};

// CMAC authenticator implementation

struct CMACAuth
{
	static const DWORD BlockSize = AESCipher::BlockSize;

	AESCipher	*cipher;
	BYTE		k1[BlockSize], k2[BlockSize];
	BYTE		pad[BlockSize];
	DWORD		pos;

	bool Init(AESCipher &c)
	{
		cipher = &c;
		Reset();
		cipher->Encrypt(pad);
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

	bool Process(void *buf, DWORD size)
	{
		for (DWORD i = 0; i < size; i++)
		{
			if (pos == BlockSize)
			{
				if (!cipher->Encrypt(pad))
					return false;
				pos = 0;
			}
			pad[pos++] ^= ((BYTE *)buf)[i];
		}
		return true;
	}

	bool Finalize(void *tag, DWORD size = BlockSize)
	{
		if (size < 4 || size > BlockSize)
		{
			sprintf_s(lastError, "Invalid tag size for CMAC (%d)", size);
			return SetLastError(lastError);
		}
		if (pos < BlockSize)
			pad[pos] ^= 0x80;
		BYTE *key = (pos < BlockSize ? k2 : k1);
		for (DWORD i = 0; i < BlockSize; i++)
			pad[i] ^= key[i];
		if (!cipher->Encrypt(pad))
			return false;
		pos = 0;
		memcpy(tag, pad, size);
		return true;
	}

private:
	static void DoubleBlock(BYTE *in, BYTE *out)
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

	static bool SetLastError(char *error)
	{
		vnclog.Print(0, _T("CMACAuth: %s\n"), error);
		throw WarningException(error);
		return false;
	}
};

// EAX cipher mode AEAD (authenticated encryption with associated data) implementation

struct EAXMode
{
	static const DWORD BlockSize = AESCipher::BlockSize;
	static const DWORD MacSize = CMACAuth::BlockSize;

	AESCipher	cipher;
	CMACAuth	auth;
	BYTE		ctr[BlockSize], pad[BlockSize];
	int 		pos;
	BYTE		tagNonce[MacSize];
	BYTE		tagAad[MacSize];

	bool Init(DWORD keySize, void *key)
	{
		if (!cipher.Init(keySize, key))
			return false;
		if (!auth.Init(cipher))
			return false;
		pos = -1;
		return true;
	}

	void SetNonce(DWORD msg)
	{
		BYTE buf[BlockSize] = { 0 };
		memcpy(buf, &msg, 4);
		SetNonce(buf, BlockSize);
	}

	void SetNonce(void *buf, DWORD size)
	{
		Mac(0, buf, size, tagNonce);
		memcpy(ctr, tagNonce, sizeof(ctr));
	}

	void SetAad(void *buf, DWORD size)
	{
		Mac(1, buf, size, tagAad);
	}

	bool Process(void *buf, DWORD size, bool encrypt = true)
	{
		if (pos < 0)
		{
			Mac(2, NULL, 0, NULL);
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
			((BYTE *)buf)[i] ^= pad[pos++];
		}
		if (encrypt && !auth.Process(buf, size))
			return false;
		return true;
	}

	bool Finalize(void *tag, DWORD size = MacSize)
	{
		if (!auth.Finalize(tag, size))
			return false;
		for (DWORD i = 0; i < size; i++)
			((BYTE *)tag)[i] ^= tagNonce[i] ^ tagAad[i];
		pos = -1;
		return true;
	}

private:
	void Mac(BYTE step, void *buf, DWORD size, void *tag)
	{
		BYTE block[BlockSize] = { 0 };
		block[BlockSize - 1] = step;
		auth.Reset();
		auth.Process(block, sizeof(block));
		auth.Process(buf, size);
		if (tag)
			auth.Finalize(tag, MacSize);
	}

	static void WrapIncr(BYTE ctr[BlockSize])
	{
		for (int i = BlockSize - 1; i >= 0; i--)
			if (++ctr[i])
				break;
	}
};

// Encryption/decryption plugin based on AES in EAX mode

struct AESEAXPlugin : IPlugin
{
	static const int BlockSize = EAXMode::BlockSize;
	static const int MacSize = EAXMode::MacSize;
	static const int AadSize = 2;

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

		inline int GetAvailable() { return max(size - pos, 0); }
	};

	EAXMode		encAead, decAead;
	DWORD		encMsg, decMsg;
	DynBuffer	encBuffer, decBuffer, decPlain;
	int			wanted;

	AESEAXPlugin(DWORD keySize, void *encKey, void *decKey)
	{
		encAead.Init(keySize, encKey);
		decAead.Init(keySize, decKey);
		encMsg = decMsg = 0;
	}

	virtual ~AESEAXPlugin() { }

	virtual BYTE *TransformBuffer(BYTE *pDataBuffer, int nDataLen, int *pnTransformedDataLen)
	{
		*pnTransformedDataLen = AadSize + nDataLen + MacSize;
		encBuffer.size = 0;
		BYTE *dst = encBuffer.EnsureFree(*pnTransformedDataLen);
		*((CARD16 *)dst) = Swap16IfLE(nDataLen);
		memcpy(dst + AadSize, pDataBuffer, nDataLen);
		encAead.SetNonce(encMsg++);
		encAead.SetAad(dst, AadSize);
		if (!encAead.Process(dst + AadSize, nDataLen))
			SetLastError("Encryption failed");
		if (!encAead.Finalize(dst + AadSize + nDataLen, MacSize))
			SetLastError("Tag failed on encryption");
		return dst;
	}

	virtual BYTE *RestoreBuffer(BYTE *pTransBuffer, int nTransDataLen, int *pnRestoredDataLen)
	{
		if (!pTransBuffer)
		{
			wanted = nTransDataLen;
			if (decPlain.GetAvailable() >= wanted)
				*pnRestoredDataLen = 0;
			else if (decBuffer.GetAvailable() < AadSize)
				*pnRestoredDataLen = AadSize - decBuffer.GetAvailable();
			else
			{
				int needed = AadSize + Swap16IfLE(*((CARD16 *)decBuffer.GetHead())) + MacSize;
				*pnRestoredDataLen = max(needed - decBuffer.GetAvailable(), 0);
			}
			BYTE *dst = decBuffer.EnsureFree(*pnRestoredDataLen);
			decBuffer.size += *pnRestoredDataLen;
			return dst;
		}
		while (decBuffer.GetAvailable() >= AadSize)
		{
			DWORD size = Swap16IfLE(*((CARD16*)decBuffer.GetHead()));
			if (AadSize + size + MacSize > (DWORD)decBuffer.GetAvailable())
				break;
			BYTE *dst = decPlain.EnsureFree(size);
			decAead.SetNonce(decMsg++);
			decAead.SetAad(decBuffer.GetHead(), AadSize);
			memcpy(dst, decBuffer.GetHead() + AadSize, size);
			if (!decAead.Process(dst, size, false))
				SetLastError("Decryption failed");
			BYTE tag[MacSize];
			if (!decAead.Finalize(tag, MacSize))
				SetLastError("Tag failed on decryption");
			if (!ArrayEqual(tag, decBuffer.GetHead() + AadSize + size, MacSize))
				SetLastError("Decryption tag does not match");
			decPlain.size += size;
			decBuffer.pos += AadSize + size + MacSize;
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
		vnclog.Print(0, _T("AESEAXPlugin: %s\n"), error);
		throw WarningException(error);
		return false;
	}
};

// RSA bases key exchange

struct RSAKEX
{
	static const int MinRsaKeyLength = 1024;
	static const int MaxRsaKeyLength = 8192;
	static const int MaxRsaKeyBytes = MaxRsaKeyLength / 8;
	static const int MaxSymKeyBytes = 256 / 8;
	static const int MaxHashBytes = 32;

	struct PubKeyBlob
	{
		BLOBHEADER  hdr;
		RSAPUBKEY   key;
		BYTE		modulus[MaxRsaKeyBytes];
	};

	struct RSAKeyInfo
	{
		DWORD		length, bytes;
		BYTE		modulus[MaxRsaKeyBytes];
		BYTE		exp[MaxRsaKeyBytes];
	};

	struct IConnection
	{
		virtual void ReadExact(char *, int) = 0;
		virtual void WriteExact(char *, int) = 0;
		virtual bool CompareFingerprint(char *hex) = 0;
		virtual void SaveFingerprint(char *hex) = 0;
	};

	IConnection	&conn;
	DWORD		keySize;
	RSAKeyInfo	serverKey, clientKey;
	HCRYPTPROV	hProv;
	HCRYPTKEY	hServerKey, hClientKey;
	HCRYPTHASH	hHash;
	BYTE		serverRandom[MaxSymKeyBytes], clientRandom[MaxSymKeyBytes];
	int			subtype;
	char		lastError[1024];

	RSAKEX(IConnection &c, DWORD keysz) : conn(c), keySize(keysz),
			hProv(0), hServerKey(0), hClientKey(0), hHash(0)
	{
		lastError[0] = 0;
	}

	~RSAKEX()
	{
		memset(&serverKey, 0, sizeof(serverKey));
		memset(&clientKey, 0, sizeof(clientKey));
		memset(&serverRandom, 0, sizeof(serverRandom));
		memset(&clientRandom, 0, sizeof(clientRandom));
		if (hHash)
			CryptDestroyHash(hHash);
		if (hClientKey)
			CryptDestroyKey(hClientKey);
		if (hServerKey)
			CryptDestroyKey(hServerKey);
		if (hProv)
			CryptReleaseContext(hProv, 0);
	}

	bool ReadPublicKey()
	{
		PubKeyBlob keyBlob;

		conn.ReadExact((char *)&serverKey.length, sizeof(serverKey.length));
		serverKey.length = Swap32IfLE(serverKey.length);
		if (serverKey.length < MinRsaKeyLength)
		{
			sprintf_s(lastError, "Server RSA key is too small (%d)", serverKey.length);
			return false;
		}
		if (serverKey.length > MaxRsaKeyLength)
		{
			sprintf_s(lastError, "Server RSA key is too big (%d)", serverKey.length);
			return false;
		}
		serverKey.bytes = (serverKey.length + 7) / 8;
		conn.ReadExact((char *)serverKey.modulus, serverKey.bytes);
		conn.ReadExact((char *)serverKey.exp, serverKey.bytes);
		for (DWORD i = 0; i < serverKey.bytes - 4; i++)
		{
			if (serverKey.exp[i] != 0)
			{
				sprintf_s(lastError, "Server RSA exponent is too big (%d)", i);
				return false;
			}
		}
		if (!hProv && !CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT))
		{
			sprintf_s(lastError, "CryptAcquireContext failed (%u)", GetLastError());
			return false;
		}
		keyBlob.hdr.bType = PUBLICKEYBLOB;
		keyBlob.hdr.bVersion = CUR_BLOB_VERSION;
		keyBlob.hdr.reserved = 0;
		keyBlob.hdr.aiKeyAlg = CALG_RSA_KEYX;
		keyBlob.key.magic = BCRYPT_RSAPUBLIC_MAGIC; // RSA1
		keyBlob.key.bitlen = serverKey.length;
		keyBlob.key.pubexp = Swap32IfLE(*((DWORD *)&serverKey.exp[serverKey.bytes - 4]));
		memcpy(keyBlob.modulus, serverKey.modulus, serverKey.bytes);
		ArraySwap(keyBlob.modulus, serverKey.bytes);
		if (!CryptImportKey(hProv, (BYTE *)&keyBlob, offsetof(PubKeyBlob, modulus) + serverKey.bytes, 0, 0, &hServerKey))
		{
			sprintf_s(lastError, "Invalid server RSA key (%u)", GetLastError());
			return false;
		}
		return true;
	}

	bool VerifyServer()
	{
		BYTE f[8];		
		WCHAR msg[1024];

		DWORD size = Swap32IfLE(serverKey.length);
		if (!HashUpdate(CALG_SHA1, &size, 4)
			|| !HashUpdate(CALG_SHA1, serverKey.modulus, serverKey.bytes)
			|| !HashUpdate(CALG_SHA1, serverKey.exp, serverKey.bytes)
			|| !HashDigest(f, sizeof(f)))
		{
			return false;
		}
		sprintf_s(hex, "%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x", f[0], f[1], f[2], f[3], f[4], f[5], f[6], f[7]);
		mn_encode(f, 8, catchphrase, 1024, "x x x. x x x. x x x\n");
		strcat(catchphrase, ".");
		vnclog.Print(0, _T("RSAKEX: Server fingerprint is %s\n"), hex);
		if (conn.CompareFingerprint(hex))
			return true;
		int nButtonPressed = 0;
		swprintf_s(msg, L"The server has provided the following identifying information:\nFingerprint: %hs\n\nCatchphrase: %hs\n\nDo you want to continue?\n", hex, catchphrase);
		TASKDIALOGCONFIG tc = { 0 };
		BOOL bPersist = FALSE;
		tc.cbSize = sizeof(tc);
		tc.dwFlags = TDF_SIZE_TO_CONTENT;
		tc.dwCommonButtons = TDCBF_OK_BUTTON | TDCBF_CANCEL_BUTTON;
		tc.pszWindowTitle = L"Warning";
		tc.hInstance = pApp->m_instance;
		tc.pszMainIcon = MAKEINTRESOURCEW(IDR_TRAY);
		tc.pszMainInstruction = L"Identity confirmation";
		tc.pszContent = msg;
		tc.pszVerificationText = L"Don't ask anymore";
		TaskDialogIndirect(&tc, &nButtonPressed, NULL, &bPersist);
		if (nButtonPressed != IDOK)
			throw QuietException("Authentication cancelled");
		if (bPersist)
			conn.SaveFingerprint(hex);
		return true;
	}

	bool WritePublicKey()
	{
		#define RSA2048BIT_KEY (RSA1024BIT_KEY << 1)
		if (!CryptGenKey(hProv, AT_KEYEXCHANGE, RSA2048BIT_KEY | CRYPT_EXPORTABLE, &hClientKey))
		{
			sprintf_s(lastError, "CryptGenKey failed (%u)", GetLastError());
			return false;
		}
		BYTE keyData[2048];
		DWORD size = sizeof(keyData);
		if (!CryptExportKey(hClientKey, NULL, PUBLICKEYBLOB, 0, keyData, &size))
		{
			sprintf_s(lastError, "CryptExportKey failed (%u)", GetLastError());
			return false;
		}
		PubKeyBlob *keyBlob = (PubKeyBlob *)keyData;
		if (keyBlob->hdr.aiKeyAlg != CALG_RSA_KEYX || keyBlob->key.magic != BCRYPT_RSAPUBLIC_MAGIC)
		{
			sprintf_s(lastError, "Invalid client key generated");
			return false;
		}
		clientKey.length = keyBlob->key.bitlen;
		clientKey.bytes = (clientKey.length + 7) / 8;
		memset(clientKey.exp, 0, clientKey.bytes);
		*((DWORD *)&clientKey.exp[clientKey.bytes - 4]) = Swap32IfLE(keyBlob->key.pubexp);
		memcpy(clientKey.modulus, keyBlob->modulus, clientKey.bytes);
		ArraySwap(clientKey.modulus, clientKey.bytes);
		DWORD keyLength = Swap32IfLE(clientKey.length);
		conn.WriteExact((char *)&keyLength, 4);
		conn.WriteExact((char *)clientKey.modulus, clientKey.bytes);
		conn.WriteExact((char *)clientKey.exp, clientKey.bytes);
		return true;
	}

	bool WriteRandom()
	{
		BYTE buffer[MaxRsaKeyBytes];
		DWORD size = keySize / 8;

		if (!CryptGenRandom(hProv, size, (BYTE *)clientRandom))
		{
			sprintf_s(lastError, "CryptGenRandom failed (%u)", GetLastError());
			return false;
		}
		memcpy(buffer, clientRandom, size);
		if (!CryptEncrypt(hServerKey, NULL, TRUE, 0, buffer, &size, sizeof(buffer)))
		{
			sprintf_s(lastError, "CryptEncrypt failed (%u)", GetLastError());
			return false;
		}
		if (size != serverKey.bytes)
		{
			sprintf_s(lastError, "Server key size doesn't match (%u vs %u)", size, serverKey.bytes);
			return false;
		}
		DWORD bufSize = Swap16IfLE(size);
		conn.WriteExact((char *)&bufSize, 2);
		ArraySwap(buffer, size);
		conn.WriteExact((char *)buffer, size);
		return true;
	}

	bool ReadRandom()
	{
		BYTE buffer[MaxRsaKeyBytes];
		DWORD size = 0;

		conn.ReadExact((char *)&size, 2);
		size = Swap16IfLE(size);
		if (size != clientKey.bytes)
		{
			sprintf_s(lastError, "Client key size doesn't match (%u vs %u)", size, clientKey.bytes);
			return false;
		}
		conn.ReadExact((char *)buffer, size);
		ArraySwap(buffer, size);
		if (!CryptDecrypt(hClientKey, NULL, TRUE, 0, (BYTE *)buffer, &size))
		{
			sprintf_s(lastError, "Cannot decrypt server random (%u)", GetLastError());
			return false;
		}
		if (size != keySize / 8)
		{
			sprintf_s(lastError, "Server random size doesn't match (%u vs %u)", size, keySize / 8);
			return false;
		}
		memcpy(serverRandom, buffer, size);
		return true;
	}

	bool SetCipher(bool &fUsePlugin, IPlugin **ppPluginInterface)
	{
		ALG_ID algId = (keySize == 128 ? CALG_SHA1 : CALG_SHA_256);
		DWORD size = keySize / 8;
		BYTE encKey[MaxSymKeyBytes], decKey[MaxSymKeyBytes];

		if (!HashUpdate(algId, serverRandom, size)
			|| !HashUpdate(algId, clientRandom, size)
			|| !HashDigest(encKey, size))
		{
			return false;
		}
		if (!HashUpdate(algId, clientRandom, size)
			|| !HashUpdate(algId, serverRandom, size)
			|| !HashDigest(decKey, size))
		{
			return false;
		}
		if (ppPluginInterface)
		{
			fUsePlugin = true;
			*ppPluginInterface = new AESEAXPlugin(keySize, encKey, decKey);
		}
		return true;
	}

	bool WriteHash()
	{
		BYTE calcHash[MaxHashBytes];
		DWORD size = sizeof(calcHash);

		if (!CalcKeysHash(clientKey, serverKey, calcHash, size))
		{
			return false;
		}
		conn.WriteExact((char *)calcHash, size);
		return true;
	}

	bool ReadHash()
	{
		BYTE calcHash[MaxHashBytes], hash[MaxHashBytes];
		DWORD size = sizeof(calcHash);

		if (!CalcKeysHash(serverKey, clientKey, calcHash, size))
		{
			return false;
		}
		conn.ReadExact((char *)hash, size);
		if (!ArrayEqual(hash, calcHash, size))
		{
			sprintf_s(lastError, "Hash does not match");
			return false;
		}
		return true;
	}
	
	bool ReadSubtype()
	{
		subtype = 0;
		conn.ReadExact((char *)&subtype, 1);
		if (subtype != secTypeRA2UserPass && subtype != secTypeRA2Pass)
		{
			sprintf_s(lastError, "Invalid subtype (%d)", subtype);
			return false;
		}
		return true;
	}

	bool WriteCredentials(TCHAR *host, int port, char *cmdlnUser, char *clearPasswd)
	{
		DWORD size;

		if (strlen(clearPasswd) == 0)
		{
			AuthDialog ad;
			if (!ad.DoDialog((subtype == secTypeRA2UserPass) ? dtUserPassRSA  : dtPassRSA, host, port, hex, catchphrase))
			{
				throw QuietException("Authentication cancelled");
			}
			if (subtype == secTypeRA2UserPass)
				strcpy(cmdlnUser, ad.m_user);
			strcpy(clearPasswd, ad.m_passwd);
		}
		size = (subtype == secTypeRA2UserPass ? (DWORD)strlen(cmdlnUser) : 0);
		conn.WriteExact((char *)&size, 1);
		conn.WriteExact(cmdlnUser, size);
		size = (DWORD)strlen(clearPasswd);
		conn.WriteExact((char *)&size, 1);
		conn.WriteExact(clearPasswd, size);
		return true;
	}

private:
	bool CalcKeysHash(RSAKeyInfo &clientKey, RSAKeyInfo &serverKey, BYTE *hash, DWORD &size)
	{
		ALG_ID algId = (keySize == 128 ? CALG_SHA1 : CALG_SHA_256);
		DWORD clientLength = Swap32IfLE(clientKey.length);
		DWORD serverLength = Swap32IfLE(serverKey.length);
		DWORD hashSize = (keySize == 128 ? 20 : 32);

		if (hashSize > size)
		{
			sprintf_s(lastError, "Keys hash buffer too small (%u vs %u)", size, hashSize);
			return false;
		}
		if (!HashUpdate(algId, &clientLength, 4)
			|| !HashUpdate(algId, clientKey.modulus, clientKey.bytes)
			|| !HashUpdate(algId, clientKey.exp, clientKey.bytes)
			|| !HashUpdate(algId, &serverLength, 4)
			|| !HashUpdate(algId, serverKey.modulus, serverKey.bytes)
			|| !HashUpdate(algId, serverKey.exp, serverKey.bytes)
			|| !HashDigest(hash, hashSize))
		{
			return false;
		}
		size = hashSize;
		return true;
	}

	bool HashUpdate(ALG_ID algId, void *data, DWORD size)
	{
		if (!hHash && !CryptCreateHash(hProv, algId, 0, 0, &hHash))
		{
			sprintf_s(lastError, "CryptCreateHash failed (%u)", GetLastError());
			return false;
		}
		if (!CryptHashData(hHash, (BYTE *)data, size, 0))
		{
			sprintf_s(lastError, "CryptHashData failed (%u)", GetLastError());
			return false;
		}
		return true;
	}

	bool HashDigest(void *out, DWORD size)
	{
		BYTE buffer[256];
		DWORD bufSize = sizeof(buffer);

		if (!CryptGetHashParam(hHash, HP_HASHVAL, (BYTE *)buffer, &bufSize, 0))
		{
			sprintf_s(lastError, "CryptGetHashParam failed (%u)", GetLastError());
			return false;
		}
		CryptDestroyHash(hHash);
		hHash = 0;
		memset(out, 0, size);
		memcpy(out, buffer, size < bufSize ? size : bufSize);
		return true;
	}
};

static const TCHAR *FingerprintSection = _T("fingerprint");

struct KEXHost : public RSAKEX::IConnection
{
	ClientConnection *pConn;
	TCHAR *key, *fname;

	KEXHost(ClientConnection *pcc, TCHAR *k, TCHAR *f) : pConn(pcc), key(k), fname(f) { }

	virtual void ReadExact(char *buf, int bytes) { pConn->ReadExact(buf, bytes); }

	virtual void WriteExact(char *buf, int bytes) { pConn->WriteExact(buf, bytes); }
	
	virtual bool CompareFingerprint(char *hex)
	{ 
		TCHAR buf[32] = { 0 };
		GetPrivateProfileString(FingerprintSection, key, NULL, buf, sizeof(buf), fname);
		return _tcscmp(hex, buf) == 0;
	}
	
	virtual void SaveFingerprint(char *hex) { WritePrivateProfileString(FingerprintSection, key, hex, fname); }
};

void ClientConnection::AuthRSAAES(int keySize, bool encrypted)
{
	TCHAR key[MAX_HOST_NAME_LEN];
	sprintf_s(key, "%s:%d", m_host, m_port);
	KEXHost host(this, key, m_opts->getDefaultOptionsFileName());
	RSAKEX st(host, (DWORD)keySize);
	if (!st.ReadPublicKey()
		|| !st.VerifyServer() 
		|| !st.WritePublicKey()
		|| !st.WriteRandom()
		|| !st.ReadRandom()
		|| !st.SetCipher(m_fUsePlugin, &m_pPluginInterface)
		|| !st.WriteHash()
		|| !st.ReadHash()
		|| !st.ReadSubtype()
		|| !st.WriteCredentials(m_host, m_port, m_cmdlnUser, m_clearPasswd)) {
		if (strlen(st.lastError))
		{
			vnclog.Print(0, _T("AuthRSAAES: %s\n"), st.lastError);
			throw WarningException(st.lastError);
		}
	}
	if (!encrypted && m_pPluginInterface)
	{
		m_fUsePlugin = false;
		delete m_pPluginInterface;
		m_pPluginInterface = NULL;
	}
}
