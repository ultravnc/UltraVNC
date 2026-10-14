// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
//
// ScramSha256.h - SCRAM-SHA-256 (RFC 7677 / RFC 5802) server + client state,
// with SCRAM-SHA-256-PLUS channel binding via tls-server-end-point
// (RFC 5929). Shared between WinVNC (server side) and the viewer.
#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace ScramSha256
{
    // Escape per RFC 5802: '=' -> "=2D", ',' -> "=2C"
    std::string EscapeName(const std::string& s);

    // Channel binding mode parsed from the GS2 header.
    enum CbFlag { CB_NONE, CB_SUPPORTED_NOT_USED, CB_USED };

    // Stored verifier, PostgreSQL text form:
    //   "<iterations>:<saltB64>:<storedKeyB64>:<serverKeyB64>"
    struct Verifier {
        std::vector<uint8_t> salt;
        uint32_t iterations = 4096;
        uint8_t storedKey[32];
        uint8_t serverKey[32];
    };
    bool MakeVerifier(const std::string& password, Verifier& v,
                      uint32_t saltLen = 16, uint32_t iterations = 4096);
    // Parse/serialize the "iter:saltB64:skB64:svkB64" text form stored by the
    // validator DLL.
    bool ParseVerifier(const std::string& text, Verifier& v);
    std::string VerifierToString(const Verifier& v);

    // ---- Server side ----
    // Step 1: parse client-first. Accepts GS2 headers:
    //   "n,,"                      -> CB_NONE
    //   "y,,"                      -> CB_SUPPORTED_NOT_USED
    //   "p=tls-server-end-point,," -> CB_USED
    // outGs2Header receives the parsed header so the server can reconstruct
    // cbind-input for verification.
    bool ServerParseClientFirst(const std::vector<uint8_t>& in,
                                std::string& outUser,
                                std::string& outClientFirstBare,
                                std::string& outClientNonce,
                                std::string& outGs2Header,
                                CbFlag& outCbFlag);
    // Step 2: build server-first message.
    bool ServerBuildFirst(const Verifier& v,
                          const std::string& clientNonce,
                          std::string& outServerFirst,
                          std::string& outFullNonce /*out*/);
    // Step 3: parse client-final, verify proof and channel binding,
    // produce server-final ("v=..."). cbData = raw tls-server-end-point hash
    // bytes obtained from the TLS session (empty when cbFlag != CB_USED).
    bool ServerVerifyFinal(const Verifier& v,
                           const std::vector<uint8_t>& in,
                           const std::string& clientFirstBare,
                           const std::string& serverFirst,
                           const std::string& fullNonce,
                           const std::string& gs2Header,
                           const std::vector<uint8_t>& cbData,
                           std::string& outServerFinal);

    // ---- Client side ----
    // cbData non-empty: GS2 header "p=tls-server-end-point,," (PLUS variant).
    // cbData empty + supportsCb: "y,," (client can bind but server offered no
    // -PLUS mechanism). Otherwise "n,,".
    bool ClientFirst(const std::string& user,
                     const std::vector<uint8_t>& cbData,
                     bool supportsCb,
                     std::string& outMsg,
                     std::string& outBare, std::string& outNonce,
                     std::string& outGs2Header);
    bool ClientFinal(const std::vector<uint8_t>& serverFirst,
                     const std::string& password,
                     const std::string& clientFirstBare,
                     const std::string& clientNonce,
                     const std::string& gs2Header,
                     const std::vector<uint8_t>& cbData,
                     std::string& outMsg,
                     std::vector<uint8_t>& outExpectedServerSig /*32*/);
    bool ClientVerifyServerFinal(const std::vector<uint8_t>& serverFinal,
                                 const uint8_t expectedSig[32]);
}
