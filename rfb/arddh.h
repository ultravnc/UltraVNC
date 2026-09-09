// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2026 UltraVNC Team Members. All Rights Reserved.
//

// Apple Remote Desktop (ARD) Diffie-Hellman client side.
//
// macOS Screen Sharing (RFB security type 30) authenticates by a DH key
// agreement where the server sends (all big-endian):
//   U16 generator, U16 keyLength (bytes), prime[ keyLength ], peer public[ keyLength ]
// and the client replies with AES-128-ECB(MD5(shared)) [ username[64] password[64] ]
// followed by the client public key.
//
// The server prime is an RFC 3526 safe prime (e.g. MODP group 16 = 4096 bits on
// current macOS), so this module provides a fixed-size arbitrary-precision
// modular exponentiation engine (Montgomery multiplication over 32-bit limbs,
// no 128-bit integer type required - MSVC compatible) supporting moduli up to
// 512 bytes (4096 bits).
//
// Windows CNG/BCrypt DH is deliberately NOT used: it only supports generator 2,
// while macOS sends generator 5 (and historically 2 / 23). The only Windows
// crypto primitives this file depends on are BCryptGenRandom.

#ifndef __RFB_ARD_DH_H__
#define __RFB_ARD_DH_H__

#ifdef __cplusplus
extern "C" {
#endif

// Perform a modular exponentiation: out = base^exp mod modulus.
// All byte buffers are big-endian.
//   base    - base value (minimal big-endian bytes)
//   baseLen - byte length of base
//   exp     - exponent (big-endian)
//   expLen  - byte length of exponent
//   mod     - modulus (big-endian)
//   modLen  - byte length of modulus (1..512)
//   out     - receives result, big-endian, padded to modLen bytes
// Returns 1 on success, 0 on invalid parameters.
int ard_modpow(const unsigned char* base, int baseLen,
               const unsigned char* exp, int expLen,
               const unsigned char* mod, int modLen,
               unsigned char* out, int outLen);

// Apple ARD client DH round:
//   clientPub = gen^priv  mod prime
//   shared    = peerPub^priv mod prime
// where priv is a fresh random value in [2, prime-2] (BCryptGenRandom).
// primeLen must be 16..512 (macOS uses 64 byte / 512-bit through 512 byte / 4096-bit).
// genLen must be >= 1 (generator value, big-endian).
// clientPubOut and sharedOut receive big-endian values padded to primeLen bytes.
// Returns 1 on success, 0 on failure.
int ard_dh_compute(const unsigned char* prime, int primeLen,
                   const unsigned char* gen, int genLen,
                   const unsigned char* peerPub, int peerPubLen,
                   unsigned char* clientPubOut,
                   unsigned char* sharedOut);

#ifdef __cplusplus
}
#endif

#endif // __RFB_ARD_DH_H__