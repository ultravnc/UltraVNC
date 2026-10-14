// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
//
#include "ScramSha256.h"
#include "BcryptHelp.h"
#include "Base64.h"
#include <cstring>
#include <cstdio>
#include <cstdlib>

namespace ScramSha256
{
    std::string EscapeName(const std::string& s)
    {
        std::string out;
        out.reserve(s.size());
        for (char c : s) {
            if (c == '=') out += "=2D";
            else if (c == ',') out += "=2C";
            else out += c;
        }
        return out;
    }

    // Parse "attr=value" pairs from a SCRAM message.
    static bool GetAttr(const std::string& msg, char attr, std::string& val)
    {
        size_t pos = 0;
        while (pos < msg.size()) {
            size_t end = msg.find(',', pos);
            if (end == std::string::npos) end = msg.size();
            if (end - pos >= 2 && msg[pos] == attr && msg[pos + 1] == '=') {
                val = msg.substr(pos + 2, end - pos - 2);
                return true;
            }
            pos = end + 1;
        }
        return false;
    }

    bool MakeVerifier(const std::string& password, Verifier& v,
                      uint32_t saltLen, uint32_t iterations)
    {
        v.salt.resize(saltLen);
        v.iterations = iterations;
        uint8_t salted[32], clientKey[32];
        return BcryptHelp::Random(v.salt.data(), saltLen) &&
               BcryptHelp::Pbkdf2Sha256(password.data(), password.size(),
                                        v.salt.data(), saltLen, iterations,
                                        salted, 32) &&
               BcryptHelp::HmacSha256(salted, 32, "Client Key", 10, clientKey) &&
               BcryptHelp::Sha256(clientKey, 32, v.storedKey) &&
               BcryptHelp::HmacSha256(salted, 32, "Server Key", 10, v.serverKey);
    }

    bool ParseVerifier(const std::string& text, Verifier& v)
    {
        // "<iterations>:<saltB64>:<storedKeyB64>:<serverKeyB64>"
        size_t p1 = text.find(':');
        if (p1 == std::string::npos) return false;
        size_t p2 = text.find(':', p1 + 1);
        if (p2 == std::string::npos) return false;
        size_t p3 = text.find(':', p2 + 1);
        if (p3 == std::string::npos) return false;

        uint32_t iters = (uint32_t)strtoul(text.substr(0, p1).c_str(), NULL, 10);
        std::vector<uint8_t> salt, sk, svk;
        if (iters == 0 ||
            !Base64::Decode(text.substr(p1 + 1, p2 - p1 - 1), salt) ||
            !Base64::Decode(text.substr(p2 + 1, p3 - p2 - 1), sk) ||
            !Base64::Decode(text.substr(p3 + 1), svk))
            return false;
        if (salt.empty() || sk.size() != 32 || svk.size() != 32)
            return false;
        v.iterations = iters;
        v.salt = salt;
        memcpy(v.storedKey, sk.data(), 32);
        memcpy(v.serverKey, svk.data(), 32);
        return true;
    }

    std::string VerifierToString(const Verifier& v)
    {
        return std::to_string(v.iterations) + ":" +
               Base64::Encode(v.salt) + ":" +
               Base64::Encode(v.storedKey, 32) + ":" +
               Base64::Encode(v.serverKey, 32);
    }

    // ---------------- Server ----------------

    bool ServerParseClientFirst(const std::vector<uint8_t>& in,
                                std::string& outUser,
                                std::string& outClientFirstBare,
                                std::string& outClientNonce,
                                std::string& outGs2Header,
                                CbFlag& outCbFlag)
    {
        std::string msg((const char*)in.data(), in.size());
        // GS2 header: "n,," or "y,," or "p=cb-type,,"
        if (msg.size() < 4) return false;
        size_t gs2End;
        if ((msg[0] == 'n' || msg[0] == 'y') && msg[1] == ',' && msg[2] == ',') {
            outCbFlag = (msg[0] == 'y') ? CB_SUPPORTED_NOT_USED : CB_NONE;
            gs2End = 3;
        } else if (msg.compare(0, 2, "p=") == 0) {
            size_t comma1 = msg.find(',', 2);
            if (comma1 == std::string::npos || comma1 + 1 >= msg.size() ||
                msg[comma1 + 1] != ',')
                return false;
            if (msg.substr(2, comma1 - 2) != "tls-server-end-point")
                return false;
            outCbFlag = CB_USED;
            gs2End = comma1 + 2;
        } else {
            return false;
        }
        outGs2Header = msg.substr(0, gs2End);
        outClientFirstBare = msg.substr(gs2End);
        if (!GetAttr(outClientFirstBare, 'n', outUser) || outUser.empty())
            return false;
        return GetAttr(outClientFirstBare, 'r', outClientNonce) &&
               !outClientNonce.empty();
    }

    bool ServerBuildFirst(const Verifier& v,
                          const std::string& clientNonce,
                          std::string& outServerFirst,
                          std::string& outFullNonce)
    {
        uint8_t snonceRaw[24];
        if (!BcryptHelp::Random(snonceRaw, sizeof(snonceRaw)))
            return false;
        outFullNonce = clientNonce + Base64::Encode(snonceRaw, sizeof(snonceRaw));
        outServerFirst = "r=" + outFullNonce +
                         ",s=" + Base64::Encode(v.salt) +
                         ",i=" + std::to_string(v.iterations);
        return true;
    }

    bool ServerVerifyFinal(const Verifier& v,
                           const std::vector<uint8_t>& in,
                           const std::string& clientFirstBare,
                           const std::string& serverFirst,
                           const std::string& fullNonce,
                           const std::string& gs2Header,
                           const std::vector<uint8_t>& cbData,
                           std::string& outServerFinal)
    {
        std::string msg((const char*)in.data(), in.size());
        // client-final-without-proof = everything before ",p=..."
        size_t ppos = msg.rfind(",p=");
        if (ppos == std::string::npos) return false;
        std::string withoutProof = msg.substr(0, ppos);

        std::string cbind, nonce, proofB64;
        if (!GetAttr(msg, 'c', cbind) || !GetAttr(msg, 'r', nonce) ||
            !GetAttr(msg.substr(ppos + 1), 'p', proofB64))
            return false;
        if (nonce != fullNonce) return false;

        // Verify channel binding: cbind-input = gs2Header + cbData
        std::vector<uint8_t> expectedCbind(gs2Header.begin(), gs2Header.end());
        expectedCbind.insert(expectedCbind.end(), cbData.begin(), cbData.end());
        std::vector<uint8_t> cb;
        if (!Base64::Decode(cbind, cb) || cb != expectedCbind)
            return false;

        std::vector<uint8_t> proof;
        if (!Base64::Decode(proofB64, proof) || proof.size() != 32)
            return false;

        std::string authMessage = clientFirstBare + "," + serverFirst +
                                  "," + withoutProof;
        uint8_t clientSig[32], clientKey[32], check[32];
        if (!BcryptHelp::HmacSha256(v.storedKey, 32,
                                    authMessage.data(), authMessage.size(),
                                    clientSig))
            return false;
        for (int i = 0; i < 32; i++)
            clientKey[i] = proof[i] ^ clientSig[i];
        if (!BcryptHelp::Sha256(clientKey, 32, check) ||
            memcmp(check, v.storedKey, 32) != 0)
            return false;

        uint8_t serverSig[32];
        if (!BcryptHelp::HmacSha256(v.serverKey, 32,
                                    authMessage.data(), authMessage.size(),
                                    serverSig))
            return false;
        outServerFinal = "v=" + Base64::Encode(serverSig, 32);
        return true;
    }

    // ---------------- Client ----------------

    bool ClientFirst(const std::string& user,
                     const std::vector<uint8_t>& cbData,
                     bool supportsCb,
                     std::string& outMsg,
                     std::string& outBare, std::string& outNonce,
                     std::string& outGs2Header)
    {
        uint8_t nonceRaw[18];
        if (!BcryptHelp::Random(nonceRaw, sizeof(nonceRaw)))
            return false;
        outNonce = Base64::Encode(nonceRaw, sizeof(nonceRaw));
        outBare = "n=" + EscapeName(user) + ",r=" + outNonce;
        if (!cbData.empty())
            outGs2Header = "p=tls-server-end-point,,";
        else if (supportsCb)
            outGs2Header = "y,,";
        else
            outGs2Header = "n,,";
        outMsg = outGs2Header + outBare;
        return true;
    }

    bool ClientFinal(const std::vector<uint8_t>& serverFirst,
                     const std::string& password,
                     const std::string& clientFirstBare,
                     const std::string& clientNonce,
                     const std::string& gs2Header,
                     const std::vector<uint8_t>& cbData,
                     std::string& outMsg,
                     std::vector<uint8_t>& outExpectedServerSig)
    {
        std::string sf((const char*)serverFirst.data(), serverFirst.size());
        std::string nonce, saltB64, iterStr;
        if (!GetAttr(sf, 'r', nonce) || !GetAttr(sf, 's', saltB64) ||
            !GetAttr(sf, 'i', iterStr))
            return false;
        // Server nonce must extend our client nonce (reflection guard).
        if (nonce.size() <= clientNonce.size() ||
            nonce.compare(0, clientNonce.size(), clientNonce) != 0)
            return false;
        std::vector<uint8_t> salt;
        if (!Base64::Decode(saltB64, salt) || salt.empty())
            return false;
        uint32_t iters = (uint32_t)strtoul(iterStr.c_str(), NULL, 10);
        if (iters == 0) return false;

        // cbind-input = gs2Header + cbData (RFC 5802 section 6)
        std::vector<uint8_t> cbindInput(gs2Header.begin(), gs2Header.end());
        cbindInput.insert(cbindInput.end(), cbData.begin(), cbData.end());
        std::string withoutProof =
            std::string("c=") + Base64::Encode(cbindInput) + ",r=" + nonce;
        std::string authMessage = clientFirstBare + "," + sf +
                                  "," + withoutProof;

        uint8_t salted[32], clientKey[32], storedKey[32];
        uint8_t clientSig[32], proof[32];
        if (!BcryptHelp::Pbkdf2Sha256(password.data(), password.size(),
                                      salt.data(), salt.size(), iters,
                                      salted, 32) ||
            !BcryptHelp::HmacSha256(salted, 32, "Client Key", 10, clientKey) ||
            !BcryptHelp::Sha256(clientKey, 32, storedKey) ||
            !BcryptHelp::HmacSha256(storedKey, 32,
                                    authMessage.data(), authMessage.size(),
                                    clientSig))
            return false;
        for (int i = 0; i < 32; i++)
            proof[i] = clientKey[i] ^ clientSig[i];

        uint8_t serverKey[32];
        outExpectedServerSig.resize(32);
        if (!BcryptHelp::HmacSha256(salted, 32, "Server Key", 10, serverKey) ||
            !BcryptHelp::HmacSha256(serverKey, 32,
                                    authMessage.data(), authMessage.size(),
                                    outExpectedServerSig.data()))
            return false;

        outMsg = withoutProof + ",p=" + Base64::Encode(proof, 32);
        return true;
    }

    bool ClientVerifyServerFinal(const std::vector<uint8_t>& serverFinal,
                                 const uint8_t expectedSig[32])
    {
        std::string msg((const char*)serverFinal.data(), serverFinal.size());
        std::string v;
        if (!GetAttr(msg, 'v', v)) return false;
        std::vector<uint8_t> sig;
        return Base64::Decode(v, sig) && sig.size() == 32 &&
               memcmp(sig.data(), expectedSig, 32) == 0;
    }
}
