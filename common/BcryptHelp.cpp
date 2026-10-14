// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
//
#include "BcryptHelp.h"

namespace BcryptHelp
{
    static BCRYPT_ALG_HANDLE GetAlg(LPCWSTR id, ULONG flags)
    {
        BCRYPT_ALG_HANDLE h = NULL;
        if (BCryptOpenAlgorithmProvider(&h, id, NULL, flags) != 0)
            return NULL;
        return h;
    }

    bool Sha256(const void* data, size_t len, uint8_t out[32])
    {
        BCRYPT_ALG_HANDLE hAlg = GetAlg(BCRYPT_SHA256_ALGORITHM, 0);
        if (!hAlg) return false;
        BCRYPT_HASH_HANDLE hHash = NULL;
        NTSTATUS st = BCryptCreateHash(hAlg, &hHash, NULL, 0, NULL, 0, 0);
        if (st == 0) {
            st = BCryptHashData(hHash, (PUCHAR)data, (ULONG)len, 0);
            if (st == 0)
                st = BCryptFinishHash(hHash, out, 32, 0);
        }
        if (hHash) BCryptDestroyHash(hHash);
        BCryptCloseAlgorithmProvider(hAlg, 0);
        return st == 0;
    }

    bool Sha256(const std::vector<uint8_t>& data, uint8_t out[32])
    {
        return Sha256(data.data(), data.size(), out);
    }

    bool HmacSha256(const void* key, size_t keyLen,
                    const void* data, size_t dataLen, uint8_t out[32])
    {
        BCRYPT_ALG_HANDLE hAlg = GetAlg(BCRYPT_SHA256_ALGORITHM, BCRYPT_ALG_HANDLE_HMAC_FLAG);
        if (!hAlg) return false;
        BCRYPT_HASH_HANDLE hHash = NULL;
        NTSTATUS st = BCryptCreateHash(hAlg, &hHash, NULL, 0,
                                     (PUCHAR)key, (ULONG)keyLen, 0);
        if (st == 0) {
            st = BCryptHashData(hHash, (PUCHAR)data, (ULONG)dataLen, 0);
            if (st == 0)
                st = BCryptFinishHash(hHash, out, 32, 0);
        }
        if (hHash) BCryptDestroyHash(hHash);
        BCryptCloseAlgorithmProvider(hAlg, 0);
        return st == 0;
    }

    bool Pbkdf2Sha256(const void* password, size_t passLen,
                      const void* salt, size_t saltLen,
                      uint64_t iterations, uint8_t* out, size_t outLen)
    {
        // PBKDF2-HMAC-SHA256, RFC 2898 section 5.2, implemented with our
        // HMAC helper (BCryptDeriveKeyPBKDF2 is unreliable across SKUs).
        // Only single-block output (outLen<=32) is needed for SCRAM keys.
        if (outLen > 32) return false;
        std::vector<uint8_t> u(saltLen + 4);
        memcpy(u.data(), salt, saltLen);
        u[saltLen + 0] = 0; u[saltLen + 1] = 0;
        u[saltLen + 2] = 0; u[saltLen + 3] = 1;   // block index 1, big-endian

        uint8_t t[32], prev[32];
        if (!HmacSha256(password, passLen, u.data(), u.size(), prev))
            return false;
        memcpy(t, prev, 32);
        for (uint64_t i = 1; i < iterations; i++) {
            if (!HmacSha256(password, passLen, prev, 32, prev))
                return false;
            for (int j = 0; j < 32; j++) t[j] ^= prev[j];
        }
        memcpy(out, t, outLen);
        return true;
    }

    bool Random(uint8_t* buf, size_t len)
    {
        return BCryptGenRandom(NULL, buf, (ULONG)len,
                               BCRYPT_USE_SYSTEM_PREFERRED_RNG) == 0;
    }
}
