// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
//
#include "Base64.h"

namespace Base64
{
    static const char* kTable =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    std::string Encode(const void* data, size_t len)
    {
        const uint8_t* p = (const uint8_t*)data;
        std::string out;
        out.reserve(((len + 2) / 3) * 4);
        for (size_t i = 0; i < len; i += 3) {
            uint32_t v = p[i] << 16;
            if (i + 1 < len) v |= p[i + 1] << 8;
            if (i + 2 < len) v |= p[i + 2];
            out += kTable[(v >> 18) & 63];
            out += kTable[(v >> 12) & 63];
            out += (i + 1 < len) ? kTable[(v >> 6) & 63] : '=';
            out += (i + 2 < len) ? kTable[v & 63] : '=';
        }
        return out;
    }

    std::string Encode(const std::vector<uint8_t>& data)
    {
        return Encode(data.data(), data.size());
    }

    static int Val(char c)
    {
        if (c >= 'A' && c <= 'Z') return c - 'A';
        if (c >= 'a' && c <= 'z') return c - 'a' + 26;
        if (c >= '0' && c <= '9') return c - '0' + 52;
        if (c == '+') return 62;
        if (c == '/') return 63;
        return -1;
    }

    bool Decode(const char* s, size_t len, std::vector<uint8_t>& out)
    {
        out.clear();
        if (len == 0) return true;
        if (len % 4 != 0) return false;
        uint32_t acc = 0; int nbits = 0;
        for (size_t i = 0; i < len; i++) {
            char c = s[i];
            if (c == '=') {
                // only allowed in last two positions
                if (i < len - 2) return false;
                continue;
            }
            int v = Val(c);
            if (v < 0) return false;
            acc = (acc << 6) | v; nbits += 6;
            if (nbits >= 8) {
                nbits -= 8;
                out.push_back((uint8_t)((acc >> nbits) & 0xFF));
            }
        }
        return true;
    }

    bool Decode(const std::string& s, std::vector<uint8_t>& out)
    {
        return Decode(s.data(), s.size(), out);
    }
}
