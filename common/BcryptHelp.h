// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
//
// BcryptHelp.h - minimal BCrypt wrappers (SHA-256, HMAC-SHA256, PBKDF2, RNG)
// used by the SASL/SCRAM implementation.
#pragma once
#include <windows.h>
#include <bcrypt.h>
#include <cstdint>
#include <cstddef>
#include <vector>

namespace BcryptHelp
{
    bool Sha256(const void* data, size_t len, uint8_t out[32]);
    bool Sha256(const std::vector<uint8_t>& data, uint8_t out[32]);
    bool HmacSha256(const void* key, size_t keyLen,
                    const void* data, size_t dataLen, uint8_t out[32]);
    bool Pbkdf2Sha256(const void* password, size_t passLen,
                      const void* salt, size_t saltLen,
                      uint64_t iterations, uint8_t* out, size_t outLen);
    bool Random(uint8_t* buf, size_t len);
}
