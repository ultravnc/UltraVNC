// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
//

#pragma once

// Server-side RSA-AES (RA2 / RA2ne) authentication.
//
// Implements the RealVNC RSA-AES security types (5/129 encrypted session,
// 6/130 encrypted handshake only) as used by TigerVNC, noVNC and wayvnc.
// The server RSA identity is the VeNCrypt TLS certificate key: the public
// modulus/exponent go on the wire, the private key decrypts the client
// random via NCryptDecrypt/PKCS1.
//
// After the random exchange all messages are AES-EAX framed
// (u16be length as AAD + ciphertext + 16-byte tag). For RA2 the EAX layer
// is left installed on the socket for the whole session via TLSCallbacks;
// for RA2ne it is removed after the credential exchange.

#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include <wincrypt.h>
#include <ncrypt.h>

#include "vsocket.h"

class SchannelTLS;
struct EAXStream;

class RSAAESAuth
{
public:
    explicit RSAAESAuth(VSocket* socket);
    ~RSAAESAuth();

    // Runs the full RA2 handshake and credential read.
    // keySize: 128 (RA2/RA2ne) or 256 (RA2_256/RA2ne_256).
    // allEncrypted: true for RA2, false for RA2ne.
    // requireUser: send secTypeRA2UserPass instead of secTypeRA2Pass.
    // On success outUsername/outPassword hold the client credentials.
    bool Authenticate(DWORD keySize, bool allEncrypted, bool requireUser,
                      std::string& outUsername, std::string& outPassword,
                      std::string& outFailureMessage);

private:
    bool LoadServerKey();
    bool SendServerPublicKey();
    bool ReadClientPublicKey();
    bool SendServerRandom();
    bool ReadClientRandom();
    bool SetCipher();
    bool SendKeyHash();
    bool ReadClientHash();
    bool SendSubtype();
    bool ReadCredentials(std::string& user, std::string& pass);

    bool RawSendAll(const BYTE* data, DWORD len);
    bool RawReadExact(BYTE* data, DWORD len);
    bool Fail(const char* msg);
    bool Fail(const char* msg, DWORD err);

    VSocket* m_socket;
    DWORD m_keySize;              // 128 or 256
    bool m_allEncrypted;
    bool m_requireUser;
    std::string m_lastError;

    SchannelTLS* m_tlsLoader;     // owned; cert loading only
    PCCERT_CONTEXT m_certCtx;     // borrowed from m_tlsLoader
    NCRYPT_KEY_HANDLE m_hServerKey;
    HCRYPTPROV_OR_NCRYPT_KEY_HANDLE m_hCertKey;
    DWORD m_certKeySpec;
    BOOL m_certKeyMustFree;
    HCRYPTPROV m_hProv;
    HCRYPTKEY m_hClientKey;
    HCRYPTHASH m_hHash;

    DWORD m_serverKeyBits;
    DWORD m_serverKeyBytes;
    std::vector<BYTE> m_serverN;  // big-endian wire form
    std::vector<BYTE> m_serverE;  // big-endian wire form
    DWORD m_clientKeyBits;
    DWORD m_clientKeyBytes;
    std::vector<BYTE> m_clientN;
    std::vector<BYTE> m_clientE;
    BYTE m_serverRandom[32];
    BYTE m_clientRandom[32];

    EAXStream* m_eax;             // owned EAX framing layer
    std::unique_ptr<VSocket::TLSCallbacks> m_callbacks;
    bool m_eaxInstalled;
};
