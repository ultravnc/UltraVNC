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
#include "DSMPlugin/DSMPlugin.h"

struct AESImpl
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

	AESImpl() : hProv(0), hKey(0) { }

	~AESImpl()
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
			vnclog.Print(0, _T("AESImpl: CryptAcquireContext failed (%d)\n"), GetLastError());
			return false;
		}
		SymKeyBlob keyBlob = { 0 };
		keyBlob.hdr.bType = PLAINTEXTKEYBLOB;
		keyBlob.hdr.bVersion = CUR_BLOB_VERSION;
		keyBlob.hdr.aiKeyAlg = (keySize == 128 ? CALG_AES_128 : CALG_AES_256);
		keyBlob.cbKeySize = keySize / 8;
		memcpy(keyBlob.key, key, keyBlob.cbKeySize);
		if (!CryptImportKey(hProv, (BYTE *)&keyBlob, sizeof(SymKeyBlob) - 32 + keyBlob.cbKeySize, NULL, 0, &hKey))
		{
			vnclog.Print(0, _T("AESImpl: CryptImportKey failed (%d)\n"), GetLastError());
			return false;
		}
		DWORD mode = CRYPT_MODE_ECB;
		if (!CryptSetKeyParam(hKey, KP_MODE, (BYTE *)&mode, 0))
		{
			vnclog.Print(0, _T("AESImpl: CryptSetKeyParam(KP_MODE) failed (%d)\n"), GetLastError());
			return false;
		}
		return true;
	}
	bool Encrypt(void *buf, DWORD size = BlockSize)
	{
		if (!CryptEncrypt(hKey, NULL, FALSE, 0, (BYTE *)buf, &size, size))
		{
			vnclog.Print(0, _T("AESImpl: CryptEncrypt failed (%d)\n"), GetLastError());
			return false;
		}
		return true;
	}
};


struct CMACImpl
{
	static const DWORD BlockSize = AESImpl::BlockSize;

	AESImpl		aes;
	BYTE		k1[BlockSize], k2[BlockSize];
	BYTE		pad[BlockSize];
	DWORD		pos;

	bool Init(DWORD keySize, void *key)
	{
		if (!aes.Init(keySize, key))
			return false;
		memset(pad, 0, sizeof(pad));
		aes.Encrypt(pad);
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
				if (!aes.Encrypt(pad))
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
			vnclog.Print(0, _T("CMACImpl: Invalid tag size for CMAC (%d)\n"), size);
			return false;
		}
		if (pos < BlockSize)
			pad[pos] ^= 0x80;
		BYTE *key = (pos < BlockSize ? k2 : k1);
		for (DWORD i = 0; i < BlockSize; i++)
			pad[i] ^= key[i];
		if (!aes.Encrypt(pad))
			return false;
		pos = 0;
		memcpy(tag, pad, size);
		return true;
	}

	void DoubleBlock(BYTE *in, BYTE *out)
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

struct AESEAXImpl
{
	static const DWORD BlockSize = AESImpl::BlockSize;
	static const DWORD MacSize = CMACImpl::BlockSize;

	AESImpl		aes;
	CMACImpl	cmac;
	BYTE		ctr[BlockSize], pad[BlockSize];
	int 		pos;
	BYTE		tagNonce[MacSize];
	BYTE		tagAad[MacSize];

	bool Init(DWORD keySize, void *key)
	{
		if (!cmac.Init(keySize, key))
			return false;
		if (!aes.Init(keySize, key))
			return false;
		pos = -1;
		return true;
	}

	void SetNonce(void *buf, DWORD size)
	{
		Mac(0, buf, size, tagNonce);
		memcpy(ctr, tagNonce, sizeof(ctr));
	}

	void SetNonce(DWORD msg)
	{
		BYTE buf[BlockSize];
		memcpy(buf, &msg, 4);
		memset(buf + 4, 0, BlockSize - 4);
		SetNonce(buf, BlockSize);
	}

	void SetAad(void* buf, DWORD size)
	{
		Mac(1, buf, size, tagAad);
	}

	bool Process(void *buf, DWORD size, bool encr = true)
	{
		if (pos < 0)
		{
			Mac(2, NULL, 0, NULL);
			pos = 0;
		}
		if (!encr && !cmac.Process(buf, size))
			return false;
		for (DWORD i = 0; i < size; i++)
		{
			if (pos == 0 || pos == BlockSize)
			{
				memcpy(pad, ctr, BlockSize);
				if (!aes.Encrypt(pad))
					return false;
				WrapIncr(ctr);
				pos = 0;
			}
			((BYTE *)buf)[i] ^= pad[pos++];
		}
		if (encr && !cmac.Process(buf, size))
			return false;
		return true;
	}

	bool Finalize(void *tag, DWORD size = MacSize)
	{
		if (!cmac.Finalize(tag, size))
			return false;
		for (DWORD i = 0; i < size; i++)
			((BYTE *)tag)[i] ^= tagNonce[i] ^ tagAad[i];
		pos = -1;
		return true;
	}

	void Mac(BYTE step, void *buf, DWORD size, void *tag)
	{
		BYTE block[BlockSize];
		memset(block, 0, BlockSize);
		block[BlockSize - 1] = step;
		cmac.Reset();
		cmac.Process(block, sizeof(block));
		cmac.Process(buf, size);
		if (tag)
			cmac.Finalize(tag, MacSize);
	}

	static void WrapIncr(BYTE ctr[BlockSize])
	{
		for (int i = BlockSize - 1; i >= 0; i--)
			if (++ctr[i])
				break;
	}
};

static void TestCmac() {
	{
		BYTE key[] = { 0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6, 0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c };
		BYTE buf[] = { 0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96, 0xe9, 0x3d, 0x7e, 0x11, 0x73, 0x93, 0x17, 0x2a };
		BYTE tag[] = { 0x07, 0x0a, 0x16, 0xb4, 0x6b, 0x4d, 0x41, 0x44, 0xf7, 0x9b, 0xdd, 0x9d, 0xd0, 0x4a, 0x28, 0x7c };
		BYTE calctag[16];

		CMACImpl cmac;
		cmac.Init(sizeof(key) * 8, key);
		cmac.Process(buf, sizeof(buf));
		cmac.Finalize(calctag, sizeof(calctag));
		for (int i = 0; i < 16; i++)
		{
			if (tag[i] != calctag[i])
			{
				break;
			}
		}
	}
	{
		BYTE key[] = { 0x83, 0x95, 0xfc, 0xf1, 0xe9, 0x5b, 0xeb, 0xd6, 0x97, 0xbd, 0x01, 0x0b, 0xc7, 0x66, 0xaa, 0xc3 };
		BYTE nonce[] = { 0x22, 0xe7, 0xad, 0xd9, 0x3c, 0xfc, 0x63, 0x93, 0xc5, 0x7e, 0xc0, 0xb3, 0xc1, 0x7d, 0x6b, 0x44 };
		BYTE aad[] = { 0x12, 0x67, 0x35, 0xfc, 0xc3, 0x20, 0xd2, 0x5a };
		BYTE plain[] = { 0xca, 0x40, 0xd7, 0x44, 0x6e, 0x54, 0x5f, 0xfa, 0xed, 0x3b, 0xd1, 0x2a, 0x74, 0x0a, 0x65, 0x9f, 0xfb, 0xbb, 0x3c, 0xea, 0xb7 };
		BYTE enc[] = { 0xcb, 0x89, 0x20, 0xf8, 0x7a, 0x6c, 0x75, 0xcf, 0xf3, 0x96, 0x27, 0xb5, 0x6e, 0x3e, 0xd1, 0x97, 0xc5, 0x52, 0xd2, 0x95, 0xa7 };
		BYTE tag[] = { 0xcf, 0xc4, 0x6a, 0xfc, 0x25, 0x3b, 0x46, 0x52, 0xb1, 0xaf, 0x37, 0x95, 0xb1, 0x24, 0xab, 0x6e };
		BYTE buf[100];
		BYTE calctag[16];

		AESEAXImpl aes;
		aes.Init(sizeof(key) * 8, key);
		aes.SetNonce(nonce, sizeof(nonce));
		aes.SetAad(aad, sizeof(aad));
		memcpy(buf, plain, sizeof(plain));
		aes.Process(buf, sizeof(plain));
		aes.Finalize(calctag, sizeof(calctag));
		for (int i = 0; i < sizeof(plain); i++)
		{
			if (buf[i] != enc[i])
			{
				break;
			}
		}
		for (int i = 0; i < 16; i++)
		{
			if (tag[i] != calctag[i])
			{
				break;
			}
		}

		aes.Init(sizeof(key) * 8, key);
		aes.SetNonce(nonce, sizeof(nonce));
		aes.SetAad(aad, sizeof(aad));
		memcpy(buf, enc, sizeof(enc));
		aes.Process(buf, sizeof(enc), false);
		aes.Finalize(calctag, sizeof(calctag));
		for (int i = 0; i < sizeof(enc); i++)
		{
			if (buf[i] != plain[i])
			{
				break;
			}
		}
		for (int i = 0; i < 16; i++)
		{
			if (tag[i] != calctag[i])
			{
				break;
			}
		}
	}
}

struct AESEAXPlugin : IPlugin
{
	static const int BlockSize = AESEAXImpl::BlockSize;
	static const int MacSize = AESEAXImpl::MacSize;
	static const int AadSize = 2;

	struct DynBuffer
	{
		BYTE* buffer;
		DWORD		pos, size, capacity;

		DynBuffer() : buffer(0), pos(0), size(0), capacity(0) { }
		~DynBuffer()
		{
			delete[] buffer;
		}

		BYTE* EnsureAvailable(DWORD avail)
		{
			if (size + avail > capacity) {
				delete[] buffer;
				buffer = new BYTE[size + avail];
				capacity = size + avail;
			}
			return buffer + size;
		}
	};

	AESEAXImpl	encAes, decAes;
	DWORD		encMsg, decMsg;
	DynBuffer	encBuffer, decBuffer;
	DWORD		wanted;

	AESEAXPlugin(DWORD keySize, void *encKey, void *decKey)
	{
		encAes.Init(keySize, encKey);
		decAes.Init(keySize, decKey);
		encMsg = decMsg = 0;
	}

	virtual ~AESEAXPlugin() { }

	virtual BYTE *TransformBuffer(BYTE *pDataBuffer, int nDataLen, int *pnTransformedDataLen)
	{
		*pnTransformedDataLen = AadSize + nDataLen + MacSize;
		encBuffer.size = 0;
		BYTE *dst = encBuffer.EnsureAvailable(*pnTransformedDataLen);
		*((CARD16 *)dst) = Swap16IfLE(nDataLen);
		memcpy(dst + AadSize, pDataBuffer, nDataLen);
		encAes.SetNonce(encMsg++);
		encAes.SetAad(dst, AadSize);
		if (!encAes.Process(dst + AadSize, nDataLen))
		{
			vnclog.Print(0, _T("AESEAXPlugin: Encryption failed\n"));
		}
		if (!encAes.Finalize(dst + AadSize + nDataLen, MacSize))
		{
			vnclog.Print(0, _T("AESEAXPlugin: Tag failed on encryption\n"));
		}
		return dst;
	}

	virtual BYTE *RestoreBuffer(BYTE *pTransBuffer, int nTransDataLen, int *pnRestoredDataLen)
	{
		if (!pTransBuffer)
		{
			wanted = nTransDataLen;
			*pnRestoredDataLen = AadSize + nTransDataLen + MacSize;
			BYTE *dst = encBuffer.EnsureAvailable(*pnRestoredDataLen);
			encBuffer.size += *pnRestoredDataLen;
			return dst;
		}
		DWORD size = Swap16IfLE(*((CARD16 *)encBuffer.buffer));
		while (AadSize + size + MacSize <= (DWORD)encBuffer.size)
		{
			BYTE *dst = decBuffer.EnsureAvailable(size);
			decAes.SetNonce(decMsg++);
			decAes.SetAad(encBuffer.buffer, AadSize);
			memcpy(dst, encBuffer.buffer + AadSize, size);
			if (!decAes.Process(dst, size, false))
			{
				vnclog.Print(0, _T("AESEAXPlugin: Decryption failed\n"));
			}
			BYTE tag[MacSize];
			if (!decAes.Finalize(tag, MacSize))
			{
				vnclog.Print(0, _T("AESEAXPlugin: Tag failed on decryption\n"));
			}
			if (memcmp(tag, encBuffer.buffer + AadSize + size, MacSize) != 0)
			{
				vnclog.Print(0, _T("AESEAXPlugin: Tag does not match\n"));
			}
			decBuffer.size += size;
			encBuffer.size -= AadSize + size + MacSize;
			if (encBuffer.size > 0)
				memcpy(encBuffer.buffer, encBuffer.buffer + AadSize + size + MacSize, encBuffer.size);
			size = (encBuffer.size >= 2 ? Swap16IfLE(*((CARD16*)encBuffer.buffer)) : 0);
		}
		if (decBuffer.size >= wanted)
		{
			*pnRestoredDataLen = wanted;
			memcpy(pTransBuffer, decBuffer.buffer, wanted);
			decBuffer.size -= wanted;
			if (decBuffer.size > 0)
				memcpy(decBuffer.buffer, decBuffer.buffer + wanted, decBuffer.size);
		}
		return NULL;
	}
};

const int secTypeRA2UserPass = 1;
const int secTypeRA2Pass = 2;

struct RSAImpl
{
	static const int MinKeyLength = 1024;
	static const int MaxKeyLength = 8192;
	static const int MaxAesKeyBytes = 256 / 8;
	static const int MaxHashBytes = 32;

	struct PubKeyBlob
	{
		BLOBHEADER  hdr;
		RSAPUBKEY   key;
		BYTE		modulus[MaxKeyLength / 8];
	};

	struct RSAKeyInfo
	{
		DWORD		length, bytes;
		BYTE		modulus[MaxKeyLength / 8];
		BYTE		exp[MaxKeyLength / 8];
	};

	ClientConnection *pConn;
	DWORD		keySize;
	RSAKeyInfo	serverKey, clientKey;
	HCRYPTPROV	hProv, hClientProv;
	HCRYPTKEY	hServerKey, hClientKey;
	HCRYPTHASH	hHash;
	BYTE		serverRandom[MaxAesKeyBytes], clientRandom[MaxAesKeyBytes];
	int			subtype;
	char		lastError[1024];

	RSAImpl(ClientConnection *pcc, DWORD keysz) : pConn(pcc), keySize(keysz),
			hProv(0), hClientProv(0), hServerKey(0), hClientKey(0), hHash(0)
	{
		lastError[0] = 0;
	}

	~RSAImpl()
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
		if (hClientProv)
			CryptReleaseContext(hClientProv, 0);
	}

	bool ReadPublicKey()
	{
		PubKeyBlob keyBlob;

		pConn->ReadExact((char *)&serverKey.length, sizeof(serverKey.length));
		serverKey.length = Swap32IfLE(serverKey.length);
		if (serverKey.length < MinKeyLength)
		{
			sprintf_s(lastError, "Server RSA key is too small (%d)", serverKey.length);
			return false;
		}
		if (serverKey.length > MaxKeyLength)
		{
			sprintf_s(lastError, "Server RSA key is too big (%d)", serverKey.length);
			return false;
		}
		serverKey.bytes = (serverKey.length + 7) / 8;
		pConn->ReadExact((char *)serverKey.modulus, serverKey.bytes);
		pConn->ReadExact((char *)serverKey.exp, serverKey.bytes);
		for (DWORD i = 0; i < serverKey.bytes - 4; i++)
		{
			if (serverKey.exp[i] != 0)
			{
				sprintf_s(lastError, "Server RSA exponent is too big (%d)", i);
				return false;
			}
		}
		if (!hProv && !CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT))
		{
			sprintf_s(lastError, "Unexpected error. CryptAcquireContext failed (%u)", GetLastError());
			return false;
		}
		keyBlob.hdr.bType = PUBLICKEYBLOB;
		keyBlob.hdr.bVersion = CUR_BLOB_VERSION;
		keyBlob.hdr.reserved = 0;
		keyBlob.hdr.aiKeyAlg = CALG_RSA_KEYX;
		keyBlob.key.magic = BCRYPT_RSAPUBLIC_MAGIC; // RSA1
		keyBlob.key.bitlen = serverKey.length;
		keyBlob.key.pubexp = Swap32IfLE(*((DWORD*)&serverKey.exp[serverKey.bytes - 4]));
		memcpy(keyBlob.modulus, serverKey.modulus, serverKey.bytes);
		SwapArray(keyBlob.modulus, serverKey.bytes);
		if (!CryptImportKey(hProv, (BYTE *)&keyBlob, offsetof(PubKeyBlob, modulus) + serverKey.bytes, 0, 0, &hServerKey))
		{
			sprintf_s(lastError, "Invalid server RSA key (%u)", GetLastError());
			return false;
		}
		return true;
	}

	bool VerifyServer()
	{
		DWORD size = Swap32IfLE(serverKey.length);
		BYTE f[8];

		if (!HashUpdate(CALG_SHA1, &size, 4)
			|| !HashUpdate(CALG_SHA1, serverKey.modulus, serverKey.bytes)
			|| !HashUpdate(CALG_SHA1, serverKey.exp, serverKey.bytes)
			|| !HashDigest(f, sizeof(f)))
		{
			return false;
		}
		vnclog.Print(0, "The server has provided the following identifying information:\n"
			"Fingerprint: %02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x\n", f[0], f[1], f[2], f[3], f[4], f[5], f[6], f[7]);
		return true;
	}

	bool WritePublicKey()
	{
		if (!hClientProv && !CryptAcquireContext(&hClientProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT))
		{
			sprintf_s(lastError, "Unexpected error. CryptAcquireContext failed (%u)", GetLastError());
			return false;
		}
		#define RSA2048BIT_KEY (RSA1024BIT_KEY << 1)
		if (!CryptGenKey(hClientProv, AT_KEYEXCHANGE, RSA2048BIT_KEY | CRYPT_EXPORTABLE, &hClientKey))
		{
			sprintf_s(lastError, "Unexpected error. CryptGenKey failed (%u)", GetLastError());
			return false;
		}
		BYTE keyData[2048] = { 0 };
		DWORD size = sizeof(keyData);
		if (!CryptExportKey(hClientKey, NULL, PUBLICKEYBLOB, 0, keyData, &size))
		{
			sprintf_s(lastError, "Unexpected error. CryptExportKey failed (%u)", GetLastError());
			return false;
		}
		PubKeyBlob *keyBlob = (PubKeyBlob*)keyData;
		if (keyBlob->hdr.aiKeyAlg != CALG_RSA_KEYX || keyBlob->key.magic != BCRYPT_RSAPUBLIC_MAGIC)
		{
			sprintf_s(lastError, "Unexpected error. Invalid client key generated");
			return false;
		}
		clientKey.length = keyBlob->key.bitlen;
		clientKey.bytes = (clientKey.length + 7) / 8;
		memset(clientKey.exp, 0, clientKey.bytes);
		*((DWORD*)&clientKey.exp[clientKey.bytes - 4]) = Swap32IfLE(keyBlob->key.pubexp);
		memcpy(clientKey.modulus, keyBlob->modulus, clientKey.bytes);
		SwapArray(clientKey.modulus, clientKey.bytes);
		DWORD keyLength = Swap32IfLE(clientKey.length);
		pConn->WriteExact((char *)&keyLength, 4);
		pConn->WriteExact((char *)clientKey.modulus, clientKey.bytes);
		pConn->WriteExact((char *)clientKey.exp, clientKey.bytes);
		return true;
	}

	bool WriteRandom()
	{
		BYTE buffer[MaxKeyLength / 8];
		DWORD size = keySize / 8;

		if (!CryptGenRandom(hProv, size, (BYTE *)clientRandom))
		{
			sprintf_s(lastError, "Unexpected error. CryptGenRandom failed (%u)", GetLastError());
			return false;
		}
		memcpy(buffer, clientRandom, size);
		if (!CryptEncrypt(hServerKey, NULL, TRUE, 0, buffer, &size, sizeof(buffer)))
		{
			sprintf_s(lastError, "Unexpected error. CryptEncrypt failed (%u)", GetLastError());
			return false;
		}
		if (size != serverKey.bytes)
		{
			sprintf_s(lastError, "Server key size doesn't match (%u vs %u)", size, serverKey.bytes);
			return false;
		}
		DWORD bufSize = Swap16IfLE(size);
		pConn->WriteExact((char *)&bufSize, 2);
		SwapArray(buffer, size);
		pConn->WriteExact((char *)buffer, size);
		return true;
	}

	bool ReadRandom()
	{
		BYTE buffer[MaxKeyLength / 8];
		DWORD size = 0;

		pConn->ReadExact((char *)&size, 2);
		size = Swap16IfLE(size);
		if (size != clientKey.bytes)
		{
			sprintf_s(lastError, "Client key size doesn't match (%u vs %u)", size, clientKey.bytes);
			return false;
		}
		pConn->ReadExact((char *)buffer, size);
		SwapArray(buffer, size);
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

	bool SetCipher(bool &fUsePlugin, IPlugin **pPluginInterface)
	{
		ALG_ID algId = (keySize == 128 ? CALG_SHA1 : CALG_SHA_256);
		DWORD size = keySize / 8;
		BYTE encKey[MaxAesKeyBytes], decKey[MaxAesKeyBytes];

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
		if (pPluginInterface)
		{
			fUsePlugin = true;
			*pPluginInterface = new AESEAXPlugin(keySize, encKey, decKey);
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
		pConn->WriteExact((char *)calcHash, size);
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
		pConn->ReadExact((char *)hash, size);
		if (memcmp(hash, calcHash, size) != 0)
		{
			sprintf_s(lastError, "Hash does not match");
			return false;
		}
		return true;
	}
	
	bool ReadSubtype()
	{
		subtype = 0;
		pConn->ReadExact((char *)&subtype, 1);
		if (subtype != secTypeRA2UserPass && subtype != secTypeRA2Pass)
		{
			sprintf_s(lastError, "Invalid subtype (%d)", subtype);
			return false;
		}
		return true;
	}

	bool WriteCredentials(char *cmdlnUser, char *clearPasswd)
	{
		DWORD size;

		if (strlen(clearPasswd) == 0)
		{
			AuthDialog ad;
			if (!ad.DoDialog(false, pConn->m_host, pConn->m_port, true))
			{
				sprintf_s(lastError, "Logon cancelled");
				return false;
			}
			strcpy(cmdlnUser, ad.m_user);
			strcpy(clearPasswd, ad.m_passwd);
		}
		size = (subtype == secTypeRA2UserPass ? (DWORD)strlen(cmdlnUser) : 0);
		if (size > 255)
		{
			sprintf_s(lastError, "User name too long (%u)", size);
			return false;
		}
		pConn->WriteExact((char *)&size, 1);
		pConn->WriteExact(cmdlnUser, size);
		size = (DWORD)strlen(clearPasswd);
		if (size > 255)
		{
			sprintf_s(lastError, "Password too long (%u)", size);
			return false;
		}
		pConn->WriteExact((char*)&size, 1);
		pConn->WriteExact(clearPasswd, size);
		return true;
	}

	bool CalcKeysHash(RSAKeyInfo &clientKey, RSAKeyInfo &serverKey, BYTE *hash, DWORD &size)
	{
		ALG_ID algId = (keySize == 128 ? CALG_SHA1 : CALG_SHA_256);
		DWORD clientLength = Swap32IfLE(clientKey.length);
		DWORD serverLength = Swap32IfLE(serverKey.length);
		DWORD hashSize = (keySize == 128 ? 20 : 32);

		if (hashSize > size)
		{
			sprintf_s(lastError, "Unexpected error. Keys hash buffer too small (%u vs %u)", size, hashSize);
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
			sprintf_s(lastError, "Unexpected error. CryptCreateHash failed (%u)", GetLastError());
			return false;
		}
		if (!CryptHashData(hHash, (BYTE *)data, size, 0))
		{
			sprintf_s(lastError, "Unexpected error. CryptHashData failed (%u)", GetLastError());
			return false;
		}
		return true;
	}

	bool HashDigest(void* out, DWORD size)
	{
		BYTE buffer[256];
		DWORD bufSize = sizeof(buffer);

		if (!CryptGetHashParam(hHash, HP_HASHVAL, (BYTE *)buffer, &bufSize, 0))
		{
			sprintf_s(lastError, "Unexpected error. CryptGetHashParam failed (%u)", GetLastError());
			return false;
		}
		CryptDestroyHash(hHash);
		hHash = 0;
		memset(out, 0, size);
		memcpy(out, buffer, size < bufSize ? size : bufSize);
		return true;
	}

	static void SwapArray(BYTE *p, DWORD size)
	{
		for (DWORD i = 0; i < size / 2; i++)
		{
			const BYTE t = p[i];
			p[i] = p[size - 1 - i];
			p[size - 1 - i] = t;
		}
	}
};

void ClientConnection::AuthRSAAES(int keySize, bool encrypted)
{
	TestCmac();
	RSAImpl st(this, (DWORD)keySize);
	if (!st.ReadPublicKey()
		|| !st.VerifyServer() 
		|| !st.WritePublicKey()
		|| !st.WriteRandom()
		|| !st.ReadRandom()
		|| !st.SetCipher(m_fUsePlugin, encrypted ? &m_pPluginInterface : NULL)
		|| !st.WriteHash()
		|| !st.ReadHash()
		|| !st.ReadSubtype()
		|| !st.WriteCredentials(m_cmdlnUser, m_clearPasswd)) {
		vnclog.Print(0, _T("AuthRSAAES: %s\n"), st.lastError);
		throw WarningException(st.lastError);
	}
}
