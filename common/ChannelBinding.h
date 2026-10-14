// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
//
// ChannelBinding.h - RFC 5929 'tls-server-end-point' channel binding.
//
// The binding data is a hash of the TLS server's certificate exactly as it
// appears in the TLS Certificate message (the DER encoding). The hash
// algorithm is selected from the certificate's signatureAlgorithm:
//   - MD5 or SHA-1 signature  -> SHA-256
//   - any other single-hash algorithm -> that algorithm
//   - unknown / multi-hash algorithms -> SHA-256 (safe default, matches
//     PostgreSQL/FreeRDP behaviour for unknown OIDs)
//
// Both sides compute the hash from the certificate itself, which avoids
// depending on SECPKG_ATTR_ENDPOINT_BINDINGS (unavailable before Windows 7
// and ambiguous about which certificate it hashes server-side).
#pragma once
#include <windows.h>
#include <wincrypt.h>
#include <vector>

namespace ChannelBinding
{
    // Compute tls-server-end-point binding data (the raw hash bytes, without
    // the "tls-server-end-point:" prefix) for the given certificate.
    bool ServerEndPointHash(PCCERT_CONTEXT cert, std::vector<BYTE>& outHash);
}
