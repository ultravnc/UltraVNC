// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
//

#pragma once

#include "SchannelTLS.h"
#include "vsocket.h"
#include <cstdint>
#include <vector>
#include <functional>
#include <memory>
#include <mutex>
#include <string>

// Server-side VeNCrypt handler.
//
// Implements the VeNCrypt 0.2 security type (rfbVeNCrypt = 19) on top of the
// existing SchannelTLS wrapper. Advertised subtypes (official numbering):
//   secTypeX509Plain (262), secTypeX509Vnc (261), secTypeTLSPlain (259),
//   secTypeTLSNone (257), plus:
//   secTypeX509SASL (264) / secTypeTLSSASL (263) -> QEMU-compatible SASL
//                            framing with SCRAM-SHA-256 / SCRAM-SHA-256-PLUS
//                            (tls-server-end-point channel binding). Offered
//                            only when the validator DLL exports sasl_lookup.
//   secTypeTokenTLS (300) -> versioned "UVNC" payload with primary-token and
//                            reconnect-session-token authentication methods.
//
// The client requests session-token issuance with a protocol flag. The server
// always returns a versioned response after successful subtype-300 auth; the
// response contains a token only when the validator supports issuance.
//
// The object owns the SchannelTLS session and the VSocket TLS callbacks for the
// lifetime of the RFB connection.

class VeNCryptAuth
{
public:
    explicit VeNCryptAuth(VSocket* socket);
    ~VeNCryptAuth();

    // Run the full VeNCrypt + token authentication flow.
    // On success the TLS callbacks are installed on the supplied VSocket.
    // Returns true on successful authentication, false on failure.
    bool Authenticate(std::string& outFailureMessage);

    uint32_t GetChosenType() const { return m_chosenType; }
    bool ShouldSendTokenResponse() const { return m_requestSessionToken; }

    // Send the subtype-300 response requested by the client. The response is
    // always emitted; unsupported/failed token issuance is encoded as flags=0,
    // length=0 so the authenticated connection remains usable.
    bool IssueAndSendSessionToken(std::string& outFailureMessage);

    // Called after the TLS handshake when the client picks secTypeX509Vnc:
    // runs the classic VNC challenge/response over the encrypted channel.
    // Set by the caller (vncClientThread::AuthVnc). If unset, subtype 261
    // is accepted but fails with "VNC auth unavailable".
    std::function<bool(std::string&)> VncAuthCallback;

private:
    bool SendVeNCryptVersion();
    bool ReadClientVersion();
    bool SendSubTypes();
    bool ReadChosenType(uint32_t& outType);
    bool PerformTLSHandshake(std::string& outFailureMessage);
    bool ReadCredentials(std::string& outUsername, std::string& outToken);
    bool ValidateToken(const char* username, const char* token, std::string& outFailureMessage);
    // QEMU-compatible SASL exchange over the TLS channel (subtypes 263/264).
    // Runs the mechlist/step framing and SCRAM-SHA-256(-PLUS) verification,
    // then returns; the caller's rfbVncAuth result doubles as the SASL
    // SecurityResult block (same u32 + reason format QEMU uses).
    bool RunSaslAuth(std::string& outFailureMessage);

    bool SendU8(uint8_t value);
    bool SendU32(uint32_t value);
    bool ReadU8(uint8_t& value);
    bool ReadU32(uint32_t& value);
    bool ReadExact(char* buf, int len);
    bool SendExact(const char* buf, int len);

    VSocket* m_socket;
    SchannelTLS m_tls;
    std::mutex m_tlsMutex; // serializes Schannel Send/Recv (context not thread-safe)
    std::unique_ptr<VSocket::TLSCallbacks> m_tlsCallbacks;
    uint32_t m_chosenType;
    std::string m_lastUsername;
    bool m_requestSessionToken;
    bool m_tlsActive;
};
