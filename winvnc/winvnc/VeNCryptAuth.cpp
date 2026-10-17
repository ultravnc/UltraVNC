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
#include "common/ScramSha256.h"
#include "common/SaslWire.h"
#include <wincrypt.h>
#include <windows.h>

extern VNCLog vnclog;

namespace
{
    constexpr uint8_t VeNCryptMajorVersion = 0;
    constexpr uint8_t VeNCryptMinorVersion = 2;
    constexpr size_t MaxUsernameLen = 256;
    constexpr size_t MaxTokenLen = 64;
    // Verifier text form stored by the validator DLL:
    // "<iterations>:<saltB64>:<storedKeyB64>:<serverKeyB64>"
    constexpr size_t MaxVerifierLen = 512;

    typedef int (__cdecl *ValidateTokenFn)(const char* user, const char* token);
    typedef int (__cdecl *IssueSessionTokenFn)(const char* user, char* outBuf, size_t outBufSize);
    typedef int (__cdecl *SaslLookupFn)(const char* user, char* outBuf, size_t outBufSize);

    // SASL framing runs over the VSocket, which already has the TLS
    // callbacks installed once PerformTLSHandshake() completes.
    class SocketTransport : public ISaslTransport
    {
    public:
        explicit SocketTransport(VSocket* socket) : m_socket(socket) {}
        bool SendAll(const void* data, size_t len) override
        {
            return m_socket->SendExact((const char*)data, (const VCard)len) != VFalse;
        }
        bool RecvAll(void* data, size_t len) override
        {
            return m_socket->ReadExact((char*)data, (const VCard)len) != VFalse;
        }
    private:
        VSocket* m_socket;
    };

    // Keep the token validator DLL loaded for the lifetime of the process. The
    // DLL stores in-memory session tokens, so unloading it between calls would
    // wipe the token table.
    HMODULE g_hTokenValidatorDll = NULL;
    std::mutex g_tokenDllMutex;

    static bool IsAbsolutePath(const char* path)
    {
        return path && ((path[0] >= 'A' && path[0] <= 'Z') || (path[0] >= 'a' && path[0] <= 'z')) &&
               path[1] == ':' && (path[2] == '\\' || path[2] == '/');
    }

    static HMODULE SafeLoadValidatorDll(const char* path)
    {
        if (!path || !path[0])
            return NULL;
        if (!IsAbsolutePath(path))
        {
            vnclog.Print(LL_CONNERR, VNCLOG("VeNCrypt: validator DLL path must be absolute (%s)\n"), path);
            return NULL;
        }
        // Use LOAD_WITH_ALTERED_SEARCH_PATH so dependent DLLs are resolved from
        // the validator DLL's directory rather than the WinVNC search path.
        return LoadLibraryExA(path, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
    }

    HMODULE GetTokenValidatorDll()
    {
        std::lock_guard<std::mutex> lock(g_tokenDllMutex);
        if (!g_hTokenValidatorDll)
        {
            const char* dllPath = SettingsManager::getInstance()->getVeNCryptTokenDLLPath();
            if (dllPath && dllPath[0])
                g_hTokenValidatorDll = SafeLoadValidatorDll(dllPath);
        }
        return g_hTokenValidatorDll;
    }

    // The validator DLL exports sasl_lookup(user, outBuf, size) when it can
    // supply SCRAM verifiers. Its presence gates offering subtypes 263/264.
    SaslLookupFn GetSaslLookup()
    {
        HMODULE hModule = GetTokenValidatorDll();
        if (!hModule)
            return nullptr;
        return (SaslLookupFn)GetProcAddress(hModule, "sasl_lookup");
    }

    bool IsTLSVeNCryptSubtype(uint32_t type)
    {
        switch (type)
        {
        case secTypeTLSNone:
        case secTypeTLSVnc:
        case secTypeTLSPlain:
        case secTypeX509None:
        case secTypeX509Vnc:
        case secTypeX509Plain:
        case secTypeTLSSASL:
        case secTypeX509SASL:
        case secTypeTLSIdent:
        case secTypeX509Ident:
        case secTypeTokenTLS:
        case secTypeX509MsLogonIII:
            return true;
        default:
            return false;
        }
    }

}

VeNCryptAuth::VeNCryptAuth(VSocket* socket)
    : m_socket(socket)
    , m_chosenType(0)
    , m_offeredCount(0)
    , m_requestSessionToken(false)
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

    // The client may only pick a subtype we actually advertised. Rechecking
    // here keeps the VeNCryptSubtypes allow-list enforced even against
    // clients that ignore the offered list.
    bool subtypeAllowed = false;
    for (int i = 0; i < m_offeredCount; ++i)
    {
        if (m_offeredTypes[i] == m_chosenType)
        {
            subtypeAllowed = true;
            break;
        }
    }
    if (!subtypeAllowed)
    {
        vnclog.Print(LL_CLIENTS, VNCLOG("VeNCrypt: client requested unadvertised subtype %u\n"), m_chosenType);
        outFailureMessage = "VeNCrypt subtype not allowed by server policy";
        return false;
    }

    if (IsTLSVeNCryptSubtype(m_chosenType) && !SendU8(0x01))
    {
        outFailureMessage = "Failed to acknowledge VeNCrypt subtype";
        return false;
    }

    if (!PerformTLSHandshake(outFailureMessage))
        return false;

    // TLSSASL/X509SASL: QEMU-compatible SASL exchange over the TLS channel.
    if (m_chosenType == secTypeTLSSASL || m_chosenType == secTypeX509SASL)
        return RunSaslAuth(outFailureMessage);

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

    if (m_chosenType == secTypeX509MsLogonIII)
    {
        vnclog.Print(LL_CLIENTS, VNCLOG("VeNCrypt: X509-MSLogonIII subtype\n"));
        if (!MsLogonIIIAuthCallback)
        {
            outFailureMessage = "MS-Logon III auth unavailable";
            return false;
        }
        return MsLogonIIIAuthCallback(outFailureMessage);
    }

    std::string username;
    std::string token;
    if (!ReadCredentials(username, token))
    {
        outFailureMessage = "Failed to read credentials";
        vnclog.Print(LL_CONNERR, VNCLOG("VeNCrypt: failed to read credentials\n"));
        return false;
    }
    m_lastUsername = username;

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

bool VeNCryptAuth::IssueAndSendSessionToken(std::string& outFailureMessage)
{
    auto sendResponse = [this](uint8_t flags, const char* token, uint16_t len) -> bool {
        uint8_t header[3] = { flags, (uint8_t)(len >> 8), (uint8_t)len };
        return SendExact((const char*)header, sizeof(header)) &&
               (len == 0 || SendExact(token, len));
    };
    auto fail = [&sendResponse, &outFailureMessage](const char* message) -> bool {
        sendResponse(0, nullptr, 0);
        outFailureMessage = message;
        return false;
    };

    if (m_lastUsername.empty())
        return fail("No username available for session token");

    HMODULE hModule = GetTokenValidatorDll();
    if (!hModule)
        return fail("Failed to load token validation DLL");

    IssueSessionTokenFn fn = (IssueSessionTokenFn)GetProcAddress(hModule, "issue_session_token");
    if (!fn)
        return fail("Token validation DLL does not export issue_session_token");

    char token[MaxTokenLen + 1] = {};
    if (!fn(m_lastUsername.c_str(), token, sizeof(token)) || token[0] == '\0')
        return fail("Token validation DLL failed to issue a session token");

    uint16_t len = (uint16_t)strlen(token);
    if (!sendResponse(uvncTokenResponseSession, token, len))
    {
        outFailureMessage = "Failed to send session token response";
        return false;
    }

    vnclog.Print(LL_CLIENTS, VNCLOG("VeNCrypt: issued %u-byte session token for user '%s'\n"),
                 (unsigned)len, m_lastUsername.c_str());
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
    // Offer one versioned private token subtype plus standard VeNCrypt types.
    // Session-token support is negotiated inside subtype 300, not by consuming
    // another private subtype number.
    CARD32 subTypes[9];
    int n = 0;

    // Admin-configured allow-list (VeNCryptSubtypes in [admin], space-separated
    // subtype numbers). Supported subtypes not in the list are not advertised;
    // an empty/unparseable list falls back to offering everything supported.
    bool allowed[1024] = {};
    const char* cfg = SettingsManager::getInstance()->getVeNCryptSubtypes();
    bool cfgEmpty = true;
    if (cfg)
    {
        const char* p = cfg;
        while (*p)
        {
            while (*p == ' ' || *p == ',' || *p == ';' || *p == '\t')
                ++p;
            char* end = nullptr;
            long v = strtol(p, &end, 10);
            if (end == p)
                break;
            if (v >= 0 && v < 1024)
            {
                allowed[v] = true;
                cfgEmpty = false;
            }
            p = end;
        }
    }
    const bool allowPlain = SettingsManager::getInstance()->getVeNCryptAllowPlainAuth() != FALSE;
    const bool requirePlus = SettingsManager::getInstance()->getVeNCryptRequireChannelBinding() != FALSE;
    auto isAllowed = [&](uint32_t t) {
        if (!cfgEmpty && !allowed[t])
            return false;
        if (!allowPlain && (t == secTypeTLSNone || t == secTypeTLSPlain || t == secTypeX509Plain))
            return false;
        if (requirePlus && t != secTypeTLSSASL && t != secTypeX509SASL)
            return false;
        return true;
    };

    // SASL subtypes are only offered when the validator DLL can supply SCRAM
    // verifiers. Third-party SASL clients (QEMU/virt-viewer, TigerVNC) pick
    // these; the UltraVNC viewer picks its own preferred type from the list.
    const bool saslAvailable = (GetSaslLookup() != nullptr);
    auto offer = [&](uint32_t t) {
        m_offeredTypes[n] = t;
        subTypes[n++] = Swap32IfLE(t);
    };

    // MS-Logon is an exclusive server authentication policy. Updated clients
    // use subtype 301; old clients require the explicit legacy-auth option.
    if (SettingsManager::getInstance()->getRequireMSLogon())
    {
        if (cfgEmpty || allowed[secTypeX509MsLogonIII])
            offer(secTypeX509MsLogonIII);
        m_offeredCount = n;
        uint8_t count = (uint8_t)n;
        return SendU8(count) && SendExact((char*)subTypes, (int)(sizeof(CARD32) * n));
    }

    if (isAllowed(secTypeTokenTLS))
        offer(secTypeTokenTLS);
    if (saslAvailable)
    {
        if (isAllowed(secTypeX509SASL))
            offer(secTypeX509SASL);
        if (isAllowed(secTypeTLSSASL))
            offer(secTypeTLSSASL);
    }
    if (isAllowed(secTypeTLSPlain))
        offer(secTypeTLSPlain);
    if (isAllowed(secTypeTLSNone))
        offer(secTypeTLSNone);
    if (isAllowed(secTypeX509Plain))
        offer(secTypeX509Plain);
    if (isAllowed(secTypeX509Vnc))
        offer(secTypeX509Vnc);

    m_offeredCount = n;
    uint8_t count = (uint8_t)n;
    return SendU8(count) && SendExact((char*)subTypes, (int)(sizeof(CARD32) * n));
}

bool VeNCryptAuth::ReadChosenType(uint32_t& outType)
{
    CARD32 value = 0;
    if (!ReadExact((char*)&value, sizeof(value)))
        return false;
    outType = Swap32IfLE(value);
    return true;
}

bool VeNCryptAuth::PerformTLSHandshake(std::string& outFailureMessage)
{
    SettingsManager* settings = SettingsManager::getInstance();

    bool certOk = false;
    const char* thumbprint = settings->getVeNCryptTLSCertThumbprint();
    if (thumbprint && thumbprint[0])
        certOk = m_tls.LoadCertByThumbprint(thumbprint);
    if (!certOk)
    {
        char generatedThumbprint[41] = {};
        certOk = m_tls.EnsureSelfSignedCert("CN=UltraVNC-VeNCrypt", "UltraVNC-VeNCrypt", generatedThumbprint);
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
    m_requestSessionToken = false;
    if (m_chosenType == secTypeTokenTLS)
    {
        uint8_t header[8] = {};
        if (!ReadExact((char*)header, sizeof(header)))
            return false;
        if (memcmp(header, "UVNC", 4) != 0 ||
            header[4] != uvncTokenProtoMajor ||
            header[5] > uvncTokenProtoMinor)
        {
            vnclog.Print(LL_CONNERR, VNCLOG("VeNCrypt: invalid UVNC token protocol marker/version\n"));
            return false;
        }
        if (header[6] != uvncTokenAuthPrimary &&
            header[6] != uvncTokenAuthSession)
        {
            vnclog.Print(LL_CONNERR, VNCLOG("VeNCrypt: unsupported UVNC token auth method %u\n"), (unsigned)header[6]);
            return false;
        }
        if (header[7] & ~uvncTokenFlagRequestSession)
        {
            vnclog.Print(LL_CONNERR, VNCLOG("VeNCrypt: unsupported UVNC token flags 0x%02x\n"), (unsigned)header[7]);
            return false;
        }
        m_requestSessionToken = (header[7] & uvncTokenFlagRequestSession) != 0;
        vnclog.Print(LL_CLIENTS, VNCLOG("VeNCrypt: UVNC token protocol %u.%u method=%u flags=0x%02x\n"),
                     (unsigned)header[4], (unsigned)header[5],
                     (unsigned)header[6], (unsigned)header[7]);
    }

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
    // The 6-64 range is the token-subtype contract. Standard Plain subtypes
    // (259/262) carry a regular password — a generic client (TigerVNC,
    // libvncclient) can't know about the token limit, so only a generous
    // sanity bound applies there and the validator DLL decides validity.
    const bool tokenSubtype = (m_chosenType == secTypeTokenTLS);
    const CARD32 maxLen = tokenSubtype ? (CARD32)MaxTokenLen : 1024;
    const CARD32 minLen = tokenSubtype ? 6u : 0u;
    if (tokenLen < minLen || tokenLen > maxLen)
    {
        vnclog.Print(LL_CONNERR, VNCLOG("VeNCrypt: bad credential length %u\n"), (unsigned)tokenLen);
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
    HMODULE hModule = GetTokenValidatorDll();
    if (!hModule)
    {
        outFailureMessage = "Failed to load token validation DLL";
        vnclog.Print(LL_CONNERR, VNCLOG("VeNCrypt: token validator DLL not loaded\n"));
        return false;
    }

    ValidateTokenFn fn = (ValidateTokenFn)GetProcAddress(hModule, "validate_token");
    if (!fn)
    {
        outFailureMessage = "Token validation DLL does not export validate_token";
        return false;
    }

    int result = fn(username, token);
    if (result == 0)
    {
        outFailureMessage = "Authentication failed";
        return false;
    }

    return true;
}

// QEMU-compatible SASL server exchange for subtypes 263/264. Wire format
// matches qemu ui/vnc-auth-sasl.c; the SecurityResult u32 (+ reason string)
// is emitted by the caller as the regular RFB security result.
bool VeNCryptAuth::RunSaslAuth(std::string& outFailureMessage)
{
    SaslLookupFn lookup = GetSaslLookup();
    if (!lookup)
    {
        outFailureMessage = "Validator DLL does not support SASL (no sasl_lookup export)";
        return false;
    }

    SocketTransport transport(m_socket);

    // Channel binding for SCRAM-SHA-256-PLUS: hash of our server certificate.
    // Offer -PLUS only when the hash is available; SCRAM-SHA-256 stays for
    // clients that do not support channel binding (QEMU, TigerVNC) unless the
    // admin has explicitly required channel binding.
    const bool requirePlus = SettingsManager::getInstance()->getVeNCryptRequireChannelBinding();
    std::vector<BYTE> cbData;
    const bool haveBinding = m_tls.GetEndpointBinding(cbData);
    std::string mechlist;
    if (requirePlus)
    {
        if (!haveBinding)
        {
            vnclog.Print(LL_CONNERR, VNCLOG("VeNCrypt SASL: SCRAM-SHA-256-PLUS required but endpoint binding unavailable\n"));
            outFailureMessage = "SCRAM-SHA-256-PLUS required but endpoint binding unavailable";
            return false;
        }
        mechlist = "SCRAM-SHA-256-PLUS";
    }
    else
    {
        mechlist = haveBinding ? "SCRAM-SHA-256-PLUS,SCRAM-SHA-256" : "SCRAM-SHA-256";
    }
    if (!haveBinding && !requirePlus)
        vnclog.Print(LL_CLIENTS, VNCLOG("VeNCrypt SASL: no endpoint binding; -PLUS not offered\n"));

    // S->C mechlist
    if (!SaslWire::WriteString(&transport, mechlist))
    {
        outFailureMessage = "Failed to send SASL mechanism list";
        return false;
    }

    // C->S mechanism name
    std::string mech;
    if (!SaslWire::ReadString(&transport, SaslWire::kMaxMechName, mech))
    {
        outFailureMessage = "Failed to read SASL mechanism";
        return false;
    }
    if (mech != "SCRAM-SHA-256-PLUS" && mech != "SCRAM-SHA-256")
    {
        outFailureMessage = "Unsupported SASL mechanism";
        vnclog.Print(LL_CONNERR, VNCLOG("VeNCrypt SASL: rejected mech '%s'\n"), mech.c_str());
        return false;
    }
    if (requirePlus && mech != "SCRAM-SHA-256-PLUS")
    {
        outFailureMessage = "SCRAM-SHA-256-PLUS required by server policy";
        vnclog.Print(LL_CONNERR, VNCLOG("VeNCrypt SASL: rejected non-PLUS mech '%s'\n"), mech.c_str());
        return false;
    }
    const bool clientPlus = (mech == "SCRAM-SHA-256-PLUS");
    if (clientPlus && !haveBinding)
    {
        outFailureMessage = "Channel binding requested but unavailable";
        return false;
    }

    // C->S client-first data block
    std::vector<uint8_t> clientFirst;
    bool isNull = false;
    if (!SaslWire::ReadData(&transport, clientFirst, isNull) || isNull)
    {
        outFailureMessage = "Failed to read SASL initial response";
        return false;
    }

    std::string user, clientFirstBare, clientNonce, gs2Header;
    ScramSha256::CbFlag cbFlag;
    if (!ScramSha256::ServerParseClientFirst(clientFirst, user, clientFirstBare,
                                           clientNonce, gs2Header, cbFlag))
    {
        outFailureMessage = "Malformed SCRAM client-first message";
        return false;
    }
    if (user.size() > MaxUsernameLen)
    {
        outFailureMessage = "SASL username too long";
        return false;
    }
    // Anti-downgrade guard: client that supports binding chose plain SCRAM
    // even though we advertised -PLUS.
    if (cbFlag == ScramSha256::CB_SUPPORTED_NOT_USED && haveBinding)
    {
        vnclog.Print(LL_CONNERR,
            VNCLOG("VeNCrypt SASL: client supports binding but chose non-PLUS (possible downgrade)\n"));
        outFailureMessage = "SCRAM channel binding downgrade rejected";
        return false;
    }
    // GS2 header/mech must agree: PLUS requires p= and p= requires PLUS.
    if (clientPlus != (cbFlag == ScramSha256::CB_USED) ||
        (cbFlag == ScramSha256::CB_USED && !haveBinding))
    {
        outFailureMessage = "SCRAM channel binding flag inconsistent with mechanism";
        return false;
    }

    // Look up the SCRAM verifier in the validator DLL.
    char verifierText[MaxVerifierLen + 1] = {};
    if (!lookup(user.c_str(), verifierText, sizeof(verifierText)) ||
        verifierText[0] == '\0')
    {
        outFailureMessage = "Authentication failed";
        vnclog.Print(LL_CONNERR, VNCLOG("VeNCrypt SASL: no verifier for user '%s'\n"), user.c_str());
        return false;
    }
    ScramSha256::Verifier verifier;
    if (!ScramSha256::ParseVerifier(verifierText, verifier))
    {
        outFailureMessage = "Validator returned malformed SCRAM verifier";
        return false;
    }
    m_lastUsername = user;

    // S->C server-first (incomplete: more data expected)
    std::string serverFirst, fullNonce;
    if (!ScramSha256::ServerBuildFirst(verifier, clientNonce, serverFirst, fullNonce) ||
        !SaslWire::WriteStep(&transport, serverFirst.data(), serverFirst.size(), false))
    {
        outFailureMessage = "Failed to send SASL server-first";
        return false;
    }

    // C->S client-final
    std::vector<uint8_t> clientFinal;
    if (!SaslWire::ReadData(&transport, clientFinal, isNull) || isNull)
    {
        outFailureMessage = "Failed to read SASL client-final";
        return false;
    }

    std::string serverFinal;
    static const std::vector<uint8_t> kEmptyCb;
    const std::vector<uint8_t>& cbIn =
        (cbFlag == ScramSha256::CB_USED) ? cbData : kEmptyCb;
    if (!ScramSha256::ServerVerifyFinal(verifier, clientFinal, clientFirstBare,
                                        serverFirst, fullNonce, gs2Header,
                                        cbIn, serverFinal))
    {
        outFailureMessage = "Authentication failed";
        vnclog.Print(LL_CONNERR, VNCLOG("VeNCrypt SASL: proof rejected for user '%s'\n"),
                     user.c_str());
        return false;
    }

    // S->C server-final, complete=1; caller emits the SecurityResult u32.
    if (!SaslWire::WriteStep(&transport, serverFinal.data(), serverFinal.size(), true))
    {
        outFailureMessage = "Failed to send SASL server-final";
        return false;
    }

    vnclog.Print(LL_CLIENTS, VNCLOG("VeNCrypt SASL: user '%s' authenticated via %s%s\n"),
                 user.c_str(), mech.c_str(),
                 cbFlag == ScramSha256::CB_USED ? " (channel-bound)" : "");
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
