// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
//

#include "stdhdrs.h"
#include "VeNCryptAuth.h"
#include "rfb.h"
#include "SettingsManager.h"
#include "vnclog.h"
#include <wincrypt.h>
#include <windows.h>

extern VNCLog vnclog;

namespace
{
    constexpr uint8_t VeNCryptMajorVersion = 0;
    constexpr uint8_t VeNCryptMinorVersion = 2;
    constexpr size_t MaxUsernameLen = 256;
    constexpr size_t MaxTokenLen = 64;

    typedef int (__cdecl *ValidateTokenFn)(const char* user, const char* token);
}

VeNCryptAuth::VeNCryptAuth(VSocket* socket)
    : m_socket(socket)
    , m_chosenType(0)
    , m_tlsActive(false)
{
}

VeNCryptAuth::~VeNCryptAuth()
{
    if (m_socket && m_tlsCallbacks)
    {
        // Remove the TLS callbacks so the socket reverts to plain TCP.
        m_socket->SetTLSCallbacks(nullptr);
    }
    if (m_tlsActive)
    {
        m_tls.Shutdown(m_socket);
        m_tlsActive = false;
    }
}

bool VeNCryptAuth::Authenticate(std::string& outFailureMessage)
{
    if (!m_socket)
    {
        outFailureMessage = "No socket";
        return false;
    }

    if (!SendVeNCryptVersion())
    {
        outFailureMessage = "Failed to send VeNCrypt version";
        return false;
    }

    if (!ReadClientVersion())
    {
        outFailureMessage = "Failed to negotiate VeNCrypt version";
        return false;
    }

    if (!SendSubTypes())
    {
        outFailureMessage = "Failed to send VeNCrypt subtypes";
        return false;
    }

    if (!ReadChosenType(m_chosenType))
    {
        outFailureMessage = "Failed to read chosen VeNCrypt subtype";
        return false;
    }

    if (m_chosenType != secTypeTokenTLS && m_chosenType != secTypeTLSPlain && m_chosenType != secTypeTLSNone && m_chosenType != secTypeX509Plain && m_chosenType != secTypeX509Vnc)
    {
        outFailureMessage = "Unsupported VeNCrypt subtype";
        return false;
    }

    if (!PerformTLSHandshake(outFailureMessage))
        return false;

    // TLSNone: encryption only, no credentials (interoperability test path)
    if (m_chosenType == secTypeTLSNone)
    {
        vnclog.Print(LL_CLIENTS, VNCLOG("VeNCrypt: TLSNone subtype - skipping token auth\n"));
        return true;
    }

    // X509Vnc: TLS channel + classic VNC challenge/response
    if (m_chosenType == secTypeX509Vnc)
    {
        vnclog.Print(LL_CLIENTS, VNCLOG("VeNCrypt: X509Vnc subtype - VNC challenge auth\n"));
        if (!VncAuthCallback)
        {
            outFailureMessage = "VNC auth unavailable";
            return false;
        }
        return VncAuthCallback(outFailureMessage);
    }

    std::string username;
    std::string token;
    if (!ReadCredentials(username, token))
    {
        outFailureMessage = "Failed to read credentials";
        vnclog.Print(LL_CONNERR, VNCLOG("VeNCrypt: failed to read credentials\n"));
        return false;
    }

    vnclog.Print(LL_CLIENTS, VNCLOG("VeNCrypt: credentials received, user='%s' userLen=%d tokenLen=%d\n"),
                 username.c_str(), (int)username.size(), (int)token.size());

    if (!ValidateToken(username.c_str(), token.c_str(), outFailureMessage))
    {
        vnclog.Print(LL_CONNERR, VNCLOG("VeNCrypt: token validation failed: %s\n"), outFailureMessage.c_str());
        return false;
    }

    vnclog.Print(LL_CLIENTS, VNCLOG("VeNCrypt: token validated OK\n"));

    return true;
}

bool VeNCryptAuth::SendVeNCryptVersion()
{
    // Server sends the highest version it supports (0.2).
    return SendU8(VeNCryptMajorVersion) && SendU8(VeNCryptMinorVersion);
}

bool VeNCryptAuth::ReadClientVersion()
{
    uint8_t major = 0, minor = 0;
    if (!ReadU8(major) || !ReadU8(minor))
        return false;

    // Client must support at least 0.2.
    uint16_t version = ((uint16_t)major << 8) | minor;
    if (version < 0x0002)
    {
        SendU8(0xFF); // not OK
        return false;
    }

    return SendU8(0x00); // OK
}

bool VeNCryptAuth::SendSubTypes()
{
    // Offer custom token-TLS plus standard TLS-plain (same wire format:
    // userLen, passLen, user, pass). Lets standard viewers (TigerVNC) connect.
    uint8_t count = 5;
    CARD32 subTypes[5] = { Swap32IfLE(secTypeTokenTLS), Swap32IfLE(secTypeTLSPlain), Swap32IfLE(secTypeTLSNone), Swap32IfLE(secTypeX509Plain), Swap32IfLE(secTypeX509Vnc) };
    return SendU8(count) && SendExact((char*)subTypes, sizeof(subTypes));
}

bool VeNCryptAuth::ReadChosenType(uint32_t& outType)
{
    CARD32 value = 0;
    if (!ReadExact((char*)&value, sizeof(value)))
        return false;
    outType = Swap32IfLE(value);
    // UltraVNC viewer expects a single 0x01 byte confirming the subtype is accepted.
    return SendU8(0x01);
}

bool VeNCryptAuth::PerformTLSHandshake(std::string& outFailureMessage)
{
    SettingsManager* settings = SettingsManager::getInstance();

    bool certOk = false;
    const char* thumbprint = settings->getVeNCryptTLSCertThumbprint();
    if (thumbprint && thumbprint[0])
        certOk = m_tls.LoadCertByThumbprint(thumbprint);
    else
    {
        char generatedThumbprint[41] = {};
        certOk = m_tls.EnsureSelfSignedCert(generatedThumbprint);
        if (certOk)
        {
            settings->setVeNCryptTLSCertThumbprint(generatedThumbprint);
            settings->save();
        }
    }

    if (!certOk)
    {
        outFailureMessage = "Failed to load or create TLS certificate";
        return false;
    }

    if (!m_tls.ServerHandshake(m_socket))
    {
        outFailureMessage = "TLS handshake failed";
        return false;
    }

    m_tlsActive = true;

    // Install TLS callbacks so all further RFB traffic is encrypted/decrypted.
    auto callbacks = std::make_unique<VSocket::TLSCallbacks>();
    SchannelTLS* tlsPtr = &m_tls;
    VSocket* socketPtr = m_socket;
    std::mutex* tlsMutex = &m_tlsMutex;

    callbacks->send = [tlsPtr, socketPtr, tlsMutex](const char* buf, int len) -> bool {
        std::lock_guard<std::mutex> lock(*tlsMutex);
        return tlsPtr->Send(socketPtr, buf, len);
    };
    callbacks->recv = [tlsPtr, socketPtr](char* buf, int len, int* got) -> bool {
        // No m_tlsMutex here: Recv blocks in recv() waiting for client data,
        // and holding the mutex would stall all sends until data arrives.
        // m_rawBuf/m_plainBuf are read-thread only; the Schannel context is
        // already serialized by m_csCrypto inside Recv/Send.
        return tlsPtr->Recv(socketPtr, buf, len, got);
    };

    m_tlsCallbacks = std::move(callbacks);
    m_socket->SetTLSCallbacks(m_tlsCallbacks.get());

    return true;
}

bool VeNCryptAuth::ReadCredentials(std::string& outUsername, std::string& outToken)
{
    CARD32 userLenBE = 0, tokenLenBE = 0;
    if (!ReadExact((char*)&userLenBE, sizeof(userLenBE)))
        return false;
    CARD32 userLen = Swap32IfLE(userLenBE);
    if (userLen == 0 || userLen > MaxUsernameLen)
    {
        vnclog.Print(LL_CONNERR, VNCLOG("VeNCrypt: bad username length %u\n"), (unsigned)userLen);
        return false;
    }

    if (!ReadExact((char*)&tokenLenBE, sizeof(tokenLenBE)))
        return false;
    CARD32 tokenLen = Swap32IfLE(tokenLenBE);
    if (tokenLen < 6 || tokenLen > MaxTokenLen)
    {
        vnclog.Print(LL_CONNERR, VNCLOG("VeNCrypt: bad credential length %u (expected 6-64)\n"), (unsigned)tokenLen);
        return false;
    }

    std::vector<char> userBuf(userLen + 1);
    if (!ReadExact(userBuf.data(), (int)userLen))
        return false;
    userBuf[userLen] = '\0';

    std::vector<char> tokenBuf(tokenLen + 1);
    if (!ReadExact(tokenBuf.data(), (int)tokenLen))
        return false;
    tokenBuf[tokenLen] = '\0';

    outUsername = userBuf.data();
    outToken = tokenBuf.data();
    return true;
}

bool VeNCryptAuth::ValidateToken(const char* username, const char* token,
                                 std::string& outFailureMessage)
{
    const char* dllPath = SettingsManager::getInstance()->getVeNCryptTokenDLLPath();
    if (!dllPath || dllPath[0] == '\0')
    {
        outFailureMessage = "No token validation DLL configured";
        return false;
    }

    HMODULE hModule = LoadLibraryA(dllPath);
    if (!hModule)
    {
        outFailureMessage = "Failed to load token validation DLL";
        vnclog.Print(LL_CONNERR, VNCLOG("VeNCrypt: LoadLibraryA failed for %s: %lu\n"),
                     dllPath, GetLastError());
        return false;
    }

    ValidateTokenFn fn = (ValidateTokenFn)GetProcAddress(hModule, "validate_token");
    if (!fn)
    {
        outFailureMessage = "Token validation DLL does not export validate_token";
        FreeLibrary(hModule);
        return false;
    }

    int result = fn(username, token);
    FreeLibrary(hModule);

    if (result == 0)
    {
        outFailureMessage = "Authentication failed";
        return false;
    }

    return true;
}

bool VeNCryptAuth::SendU8(uint8_t value)
{
    return SendExact((char*)&value, sizeof(value));
}

bool VeNCryptAuth::SendU32(uint32_t value)
{
    CARD32 be = Swap32IfLE(value);
    return SendExact((char*)&be, sizeof(be));
}

bool VeNCryptAuth::ReadU8(uint8_t& value)
{
    return ReadExact((char*)&value, sizeof(value));
}

bool VeNCryptAuth::ReadU32(uint32_t& value)
{
    CARD32 be = 0;
    if (!ReadExact((char*)&be, sizeof(be)))
        return false;
    value = Swap32IfLE(be);
    return true;
}

bool VeNCryptAuth::ReadExact(char* buf, int len)
{
    // VSocket functions return 1 on success while VTrue is -1: compare to VFalse.
    return m_socket->ReadExact(buf, (const VCard)len) != VFalse;
}

bool VeNCryptAuth::SendExact(const char* buf, int len)
{
    bool ok = m_socket->SendExact(buf, (const VCard)len) != VFalse;
    if (!ok)
        vnclog.Print(LL_CONNERR, VNCLOG("VeNCrypt: SendExact failed len=%d wsa=%d\n"), len, WSAGetLastError());
    return ok;
}
