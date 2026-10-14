// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
//
// SaslWire.h - QEMU-compatible SASL-over-VNC wire framing
// (matches qemu ui/vnc-auth-sasl.c). Transport-agnostic: callers supply an
// ISaslTransport that reads/writes on the already-encrypted channel
// (VSocket with TLS callbacks on the server side, ClientConnection with the
// TLS plugin installed on the viewer side).
//
// Wire format:
//   S->C  u32BE mechlistLen + mechlist   (COMMA separated, no NUL)
//   C->S  u32BE mechLen + mechName       (raw, no NUL)
//   C->S  u32BE dataLen + data           (dataLen==0 -> NULL; else last byte
//                                       MUST be '\0' padding, stripped by peer)
//   S->C  u32BE outLen(+1 incl NUL) + data + u8'\0'   (or u32 0 if no data)
//         u8 complete                    (0=continue -> another C->S data block,
//                                       1=done -> u32BE SecurityResult follows)
#pragma once
#include <cstdint>
#include <string>
#include <vector>

// Minimal blocking byte channel used by the SASL framing helpers.
class ISaslTransport
{
public:
    virtual ~ISaslTransport() {}
    virtual bool SendAll(const void* data, size_t len) = 0;
    virtual bool RecvAll(void* data, size_t len) = 0;
};

namespace SaslWire
{
    const uint32_t kMaxSaslData = 1024 * 1024;  // QEMU SASL_DATA_MAX_LEN
    const uint32_t kMaxMechName = 100;          // QEMU mechname limit

    bool WriteU8(ISaslTransport* ch, uint8_t v);
    bool WriteU32(ISaslTransport* ch, uint32_t v);
    bool ReadU8(ISaslTransport* ch, uint8_t& v);
    bool ReadU32(ISaslTransport* ch, uint32_t& v);

    // u32 len + string, no NUL terminator. Used for mechlist and mechname.
    bool WriteString(ISaslTransport* ch, const char* s, size_t len);
    bool WriteString(ISaslTransport* ch, const std::string& s);
    bool ReadString(ISaslTransport* ch, uint32_t maxLen, std::string& out);

    // SASL data block: u32(len+1) + data + u8 '\0'; len==0 encodes NULL.
    bool WriteData(ISaslTransport* ch, const void* data, size_t len);
    bool WriteData(ISaslTransport* ch, const std::string& s);
    // Reads a data block. isNull=true when peer sent u32 0 (NULL data).
    // Strips and validates the trailing NUL (fails if absent, like QEMU).
    bool ReadData(ISaslTransport* ch, std::vector<uint8_t>& out, bool& isNull);
    bool ReadDataString(ISaslTransport* ch, std::string& out, bool& isNull);

    // Server step reply: data block + u8 complete flag.
    bool WriteStep(ISaslTransport* ch, const void* data, size_t len, bool complete);
    bool ReadStep(ISaslTransport* ch, std::vector<uint8_t>& out, bool& isNull, bool& complete);
}
