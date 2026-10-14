// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
//
#include "SaslWire.h"

namespace SaslWire
{
    bool WriteU8(ISaslTransport* ch, uint8_t v)
    {
        return ch->SendAll(&v, 1);
    }

    bool WriteU32(ISaslTransport* ch, uint32_t v)
    {
        uint8_t b[4] = {(uint8_t)(v >> 24), (uint8_t)(v >> 16),
                        (uint8_t)(v >> 8), (uint8_t)v};
        return ch->SendAll(b, 4);
    }

    bool ReadU8(ISaslTransport* ch, uint8_t& v)
    {
        return ch->RecvAll(&v, 1);
    }

    bool ReadU32(ISaslTransport* ch, uint32_t& v)
    {
        uint8_t b[4];
        if (!ch->RecvAll(b, 4)) return false;
        v = ((uint32_t)b[0] << 24) | ((uint32_t)b[1] << 16) |
            ((uint32_t)b[2] << 8) | (uint32_t)b[3];
        return true;
    }

    bool WriteString(ISaslTransport* ch, const char* s, size_t len)
    {
        return WriteU32(ch, (uint32_t)len) && (len == 0 || ch->SendAll(s, len));
    }

    bool WriteString(ISaslTransport* ch, const std::string& s)
    {
        return WriteString(ch, s.data(), s.size());
    }

    bool ReadString(ISaslTransport* ch, uint32_t maxLen, std::string& out)
    {
        uint32_t len;
        if (!ReadU32(ch, len) || len > maxLen) return false;
        out.assign(len, '\0');
        return len == 0 || ch->RecvAll(&out[0], len);
    }

    bool WriteData(ISaslTransport* ch, const void* data, size_t len)
    {
        if (len == 0)
            return WriteU32(ch, 0);          // NULL data
        uint8_t nul = 0;
        return WriteU32(ch, (uint32_t)len + 1) &&
               ch->SendAll(data, len) &&
               ch->SendAll(&nul, 1);
    }

    bool WriteData(ISaslTransport* ch, const std::string& s)
    {
        return WriteData(ch, s.data(), s.size());
    }

    bool ReadData(ISaslTransport* ch, std::vector<uint8_t>& out, bool& isNull)
    {
        uint32_t len;
        if (!ReadU32(ch, len) || len > kMaxSaslData) return false;
        isNull = (len == 0);
        out.clear();
        if (isNull) return true;
        out.resize(len);
        if (!ch->RecvAll(out.data(), len)) { out.clear(); return false; }
        if (out.back() != 0) { out.clear(); return false; }  // QEMU requires NUL pad
        out.pop_back();
        return true;
    }

    bool ReadDataString(ISaslTransport* ch, std::string& out, bool& isNull)
    {
        std::vector<uint8_t> buf;
        if (!ReadData(ch, buf, isNull)) return false;
        out.assign((const char*)buf.data(), buf.size());
        return true;
    }

    bool WriteStep(ISaslTransport* ch, const void* data, size_t len, bool complete)
    {
        return WriteData(ch, data, len) && WriteU8(ch, complete ? 1 : 0);
    }

    bool ReadStep(ISaslTransport* ch, std::vector<uint8_t>& out, bool& isNull, bool& complete)
    {
        uint8_t c;
        return ReadData(ch, out, isNull) && ReadU8(ch, c) &&
               (complete = (c != 0), true);
    }
}
