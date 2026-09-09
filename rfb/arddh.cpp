// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2026 UltraVNC Team Members. All Rights Reserved.
//

#include "arddh.h"

#include <windows.h>
#include <bcrypt.h>
#include <string.h>

// Fixed-size arbitrary precision arithmetic for the Apple ARD D-H exchange.
//
// Numbers are stored radix 2^32, least significant limb first. Products are
// accumulated in unsigned 64-bit words (MSVC has no __int128, so we do not
// try to multiply full 64-bit limbs). The modulus is limited to 512 bytes
// (4096 bits) which is the maximum macOS Screen Sharing uses (RFC 3526 group
// 16). Montgomery multiplication is used for modular exponentiation.

#define ARD_MAX_LIMBS 128 // 512 bytes = 128 * 32-bit limbs

static void ar_zero(unsigned int v[ARD_MAX_LIMBS]) {
    memset(v, 0, sizeof(unsigned int) * ARD_MAX_LIMBS);
}

static void ar_copy(unsigned int dst[ARD_MAX_LIMBS], const unsigned int src[ARD_MAX_LIMBS], int n) {
    memcpy(dst, src, sizeof(unsigned int) * n);
}

// Number of significant limbs (drops leading zero limbs, min 1).
static int ar_used(const unsigned int a[ARD_MAX_LIMBS], int n) {
    int i = n;
    while (i > 1 && a[i - 1] == 0) i--;
    return i;
}

// Compares |a| and |b| (both width n). Returns -1, 0 or 1.
static int ar_cmp(const unsigned int a[ARD_MAX_LIMBS], const unsigned int b[ARD_MAX_LIMBS], int n) {
    for (int i = n - 1; i >= 0; i--) {
        if (a[i] != b[i]) return a[i] < b[i] ? -1 : 1;
    }
    return 0;
}

// a -= b with width w. Requires a >= b.
static void ar_sub_w(unsigned int* a, const unsigned int* b, int w) {
    unsigned long long borrow = 0;
    for (int i = 0; i < w; i++) {
        unsigned long long x = (unsigned long long)a[i] - (unsigned long long)b[i] - borrow;
        a[i] = (unsigned int)x;
        borrow = (x >> 32) & 1;
    }
}

// a -= b (both width n). Requires a >= b.
static void ar_sub(unsigned int a[ARD_MAX_LIMBS], const unsigned int b[ARD_MAX_LIMBS], int n) {
    ar_sub_w(a, b, n);
}

// a -= b where b is a small scalar (width n). Requires a >= b.
static void ar_sub_small(unsigned int a[ARD_MAX_LIMBS], unsigned long long b, int n) {
    unsigned long long borrow = 0;
    for (int i = 0; i < n; i++) {
        unsigned long long x = (unsigned long long)a[i] - (b & 0xFFFFFFFFULL) - borrow;
        a[i] = (unsigned int)x;
        borrow = (x >> 32) & 1;
        b >>= 32;
        if (b == 0 && borrow == 0) return;
    }
}

// a = b << 1 mod m (modulus m, width n). Requires b < m. m odd/normalised.
static void ar_double_mod(unsigned int out[ARD_MAX_LIMBS],
                          const unsigned int in[ARD_MAX_LIMBS],
                          const unsigned int m[ARD_MAX_LIMBS], int n) {
    unsigned int t[ARD_MAX_LIMBS + 1];
    memset(t, 0, sizeof(t));
    unsigned int carry = 0;
    for (int i = 0; i < n; i++) {
        unsigned int x = in[i];
        t[i] = (x << 1) | carry;
        carry = x >> 31;
    }
    t[n] = carry;

    // t is now at most 2m-2; subtract m once if t >= m.
    int ge = 1;
    if (t[n] == 0) {
        ge = (ar_cmp(t, m, n) >= 0) ? 1 : 0;
    }
    if (ge) {
        unsigned int mp[ARD_MAX_LIMBS + 1];
        memset(mp, 0, sizeof(mp));
        ar_copy(mp, m, n);
        ar_sub_w(t, mp, n + 1);
    }
    ar_copy(out, t, n);
}

// r = a * b mod m using Montgomery multiplication (canonical CIOS).
// Requires a < m and b < m. m must be odd. inv = -m^{-1} mod 2^32.
static void ar_mont_mul(unsigned int r[ARD_MAX_LIMBS],
                        const unsigned int a[ARD_MAX_LIMBS],
                        const unsigned int b[ARD_MAX_LIMBS],
                        const unsigned int m[ARD_MAX_LIMBS],
                        unsigned int inv, int n) {
    const unsigned long long MASK = 0xFFFFFFFFULL;
    unsigned long long T[ARD_MAX_LIMBS + 2];
    memset(T, 0, sizeof(T));

    for (int i = 0; i < n; i++) {
        // T += a[i] * b
        unsigned long long C = 0;
        for (int j = 0; j < n; j++) {
            unsigned long long x = T[j] + (unsigned long long)a[i] * b[j] + C;
            T[j] = x & MASK;
            C = x >> 32;
        }
        {
            unsigned long long x = T[n] + C;
            T[n] = x & MASK;
            T[n + 1] += x >> 32;
        }

        // u = (T[0] * inv) mod 2^32 : chosen so T + u*m is divisible by 2^32
        unsigned int u = (unsigned int)((T[0] * (unsigned long long)inv) & MASK);

        // T += u * m
        C = 0;
        for (int j = 0; j < n; j++) {
            unsigned long long x = T[j] + (unsigned long long)u * m[j] + C;
            T[j] = x & MASK;
            C = x >> 32;
        }
        {
            unsigned long long x = T[n] + C;
            T[n] = x & MASK;
            T[n + 1] += x >> 32;
        }

        // shift right by one 32-bit word
        for (int j = 0; j <= n; j++) {
            T[j] = T[j + 1];
        }
        T[n + 1] = 0;
    }

    // result is T[0..n-1] + T[n]*2^(32n), which is < 2*m.
    // Reduce with a single subtraction at full (n+1)-word width.
    unsigned int t1[ARD_MAX_LIMBS + 1];
    memset(t1, 0, sizeof(t1));
    for (int i = 0; i <= n; i++) {
        t1[i] = (unsigned int)T[i];
    }
    unsigned int mm[ARD_MAX_LIMBS + 1];
    memset(mm, 0, sizeof(mm));
    ar_copy(mm, m, n);
    if (ar_cmp(t1, mm, n + 1) >= 0) {
        ar_sub_w(t1, mm, n + 1);
    }
    ar_copy(r, t1, n);
}

// Computes R = 2^(32*n) mod m and R2 = 2^(64*n) mod m by repeated doubling.
static void ar_compute_R(const unsigned int m[ARD_MAX_LIMBS], int n,
                         unsigned int R[ARD_MAX_LIMBS], unsigned int R2[ARD_MAX_LIMBS]) {
    unsigned int cur[ARD_MAX_LIMBS];
    ar_zero(cur);
    cur[0] = 1;

    for (unsigned long long i = 0; i < (unsigned long long)n * 64; i++) {
        ar_double_mod(cur, cur, m, n);
        if (i == (unsigned long long)n * 32 - 1) {
            ar_copy(R, cur, n);
        }
    }
    ar_copy(R2, cur, n);
}

// Converts a big-endian byte string into limbs.
static void ar_from_be(unsigned int out[ARD_MAX_LIMBS], const unsigned char* in, int inLen) {
    ar_zero(out);
    for (int i = 0; i < inLen; i++) {
        int limb = inLen - 1 - i;
        out[limb / 4] |= ((unsigned int)in[i]) << (8 * (limb % 4));
    }
}

// Writes the limb value into a big-endian buffer of exactly 'len' bytes (right aligned,
// zero left padded). Requires the value to fit in 'len' bytes.
static void ar_to_be(unsigned char* out, const unsigned int a[ARD_MAX_LIMBS], int n, int len) {
    memset(out, 0, len);
    int bytes = n * 4;
    for (int i = 0; i < bytes && i < len; i++) {
        int limb = i / 4;
        int sh = 8 * (i % 4);
        out[len - 1 - i] = (unsigned char)((a[limb] >> sh) & 0xFF);
    }
}

// Computes -a^{-1} mod 2^32 via Newton iteration. 'a' must be odd.
static unsigned int ar_inv32(unsigned int a) {
    unsigned long long inv = 1;
    for (int i = 0; i < 5; i++) {
        inv = (inv * (2 - (unsigned long long)a * inv)) & 0xFFFFFFFFULL;
    }
    return (unsigned int)((0x100000000ULL - inv) & 0xFFFFFFFFULL); // -inv mod 2^32
}

// Left-to-right binary exponentiation using Montgomery multiplication.
// result = base^exp mod m. m must be odd.
static void ar_modpow(unsigned int res[ARD_MAX_LIMBS],
                      const unsigned int base[ARD_MAX_LIMBS],
                      const unsigned int exp[ARD_MAX_LIMBS],
                      int exp_used,
                      const unsigned int m[ARD_MAX_LIMBS],
                      unsigned int inv, int n) {
    unsigned int R[ARD_MAX_LIMBS], R2[ARD_MAX_LIMBS];
    ar_compute_R(m, n, R, R2);

    // convert base to Montgomery form: base * R mod m
    unsigned int baseR[ARD_MAX_LIMBS];
    ar_mont_mul(baseR, base, R2, m, inv, n);

    // start with "1" in Montgomery domain (= R mod m)
    unsigned int one[ARD_MAX_LIMBS];
    ar_copy(one, R, n);

    bool started = false;
    for (int i = exp_used * 32 - 1; i >= 0; i--) {
        unsigned int bit = (exp[i / 32] >> (i % 32)) & 1;
        if (!started) {
            if (bit == 0) continue;
            started = true;
            ar_copy(one, baseR, n); // acc = base^1 (first set bit)
            continue;
        }
        ar_mont_mul(one, one, one, m, inv, n);
        if (bit) {
            ar_mont_mul(one, one, baseR, m, inv, n);
        }
    }

    // convert back out of the Montgomery domain: montMul(value, 1) = value * R^{-1}
    unsigned int one_normal[ARD_MAX_LIMBS];
    ar_zero(one_normal);
    one_normal[0] = 1;
    ar_mont_mul(res, one, one_normal, m, inv, n);
}

int ard_modpow(const unsigned char* base, int baseLen,
               const unsigned char* exp, int expLen,
               const unsigned char* mod, int modLen,
               unsigned char* out, int outLen) {
    if (!base || baseLen < 1 || !exp || expLen < 1 || !mod || modLen < 1 || !out) return 0;
    if (modLen > ARD_MAX_LIMBS * 4) return 0;
    if (outLen < modLen) return 0;

    int n = (modLen + 3) / 4;

    unsigned int m[ARD_MAX_LIMBS];
    unsigned int b[ARD_MAX_LIMBS];
    unsigned int e[ARD_MAX_LIMBS];
    ar_from_be(m, mod, modLen);
    ar_from_be(b, base, baseLen);
    ar_from_be(e, exp, expLen);

    // modulus must be odd for Montgomery
    if ((m[0] & 1) == 0) return 0;
    // modulus must be > 1
    if (ar_used(m, n) == 1 && m[0] == 1) return 0;
    // base must be < modulus (CIOS precondition); reject larger values.
    for (int i = n; i < ARD_MAX_LIMBS; i++) {
        if (b[i] != 0) return 0;
    }
    if (ar_cmp(b, m, n) >= 0) return 0;

    unsigned int inv = ar_inv32(m[0]);
    unsigned int res[ARD_MAX_LIMBS];
    int e_used = ar_used(e, (expLen + 3) / 4);
    ar_modpow(res, b, e, e_used, m, inv, n);

    ar_to_be(out, res, n, modLen);
    return 1;
}

static int ar_random(unsigned char* buf, int len) {
    return (BCryptGenRandom(NULL, buf, len, BCRYPT_USE_SYSTEM_PREFERRED_RNG) == 0) ? 1 : 0;
}

int ard_dh_compute(const unsigned char* prime, int primeLen,
                   const unsigned char* gen, int genLen,
                   const unsigned char* peerPub, int peerPubLen,
                   unsigned char* clientPubOut,
                   unsigned char* sharedOut) {
    if (!prime || !gen || !peerPub || !clientPubOut || !sharedOut) return 0;
    if (primeLen < 16 || primeLen > ARD_MAX_LIMBS * 4) return 0;
    if (genLen < 1 || genLen > 64) return 0;
    if (peerPubLen != primeLen) return 0;

    int n = (primeLen + 3) / 4;

    unsigned int p[ARD_MAX_LIMBS];
    unsigned int g[ARD_MAX_LIMBS];
    unsigned int peer[ARD_MAX_LIMBS];
    ar_from_be(p, prime, primeLen);
    ar_from_be(g, gen, genLen);
    ar_from_be(peer, peerPub, primeLen);

    // modulus must be odd
    if ((p[0] & 1) == 0) return 0;
    // generator in [2, p-2]
    if (ar_used(g, n) == 1 && g[0] < 2) return 0;
    if (ar_cmp(g, p, n) >= 0) return 0;
    // peer public key in [1, p-1]
    if (ar_cmp(peer, p, n) >= 0) return 0;
    if (ar_used(peer, n) == 1 && peer[0] == 0) return 0;

    unsigned int inv = ar_inv32(p[0]);

    // random private exponent x in [2, p-2]; retry until in range.
    unsigned int pm1[ARD_MAX_LIMBS];
    ar_copy(pm1, p, n);
    ar_sub_small(pm1, 1, n); // p-1

    unsigned char xbuf[ARD_MAX_LIMBS * 4];
    unsigned int x[ARD_MAX_LIMBS];
    int attempts = 0;
    do {
        if (!ar_random(xbuf, primeLen)) return 0;
        ar_from_be(x, xbuf, primeLen);
        attempts++;
    } while (ar_cmp(x, pm1, n) >= 0 ||
             (ar_used(x, n) == 1 && x[0] < 2));
    // p is a 4096-bit (or larger) safe prime so the rejection rate is negligible.

    unsigned int clientPub[ARD_MAX_LIMBS];
    unsigned int shared[ARD_MAX_LIMBS];

    ar_modpow(clientPub, g, x, ar_used(x, n), p, inv, n);
    ar_modpow(shared, peer, x, ar_used(x, n), p, inv, n);

    ar_to_be(clientPubOut, clientPub, n, primeLen);
    ar_to_be(sharedOut, shared, n, primeLen);

    // zero secrets
    memset(xbuf, 0, sizeof(xbuf));
    memset(x, 0, sizeof(x));

    return 1;
}