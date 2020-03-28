/* insert_string_acle.c -- insert_string variant using ACLE's CRC instructions
 *
 * Copyright (C) 1995-2013 Jean-loup Gailly and Mark Adler
 * For conditions of distribution and use, see copyright notice in zlib.h
 *
 */

//#include "zbuild.h"
#ifndef _MSC_VER
#include <arm_acle.h> // for __crc32w
#endif

#include "deflate.h"
#include "update_hash.h"

/* ===========================================================================
 * Insert string str in the dictionary and set match_head to the previous head
 * of the hash chain (the most recent string with same hash key). Return
 * the previous length of the hash chain.
 * IN  assertion: all calls to to INSERT_STRING are made with consecutive
 *    input characters and the first MIN_MATCH bytes of str are valid
 *    (except for the last MIN_MATCH-1 bytes of the input file).
 */
Pos insert_string_acle(deflate_state *const s, const Pos str, unsigned int count) {
    Pos p, lp, ret;

    if (unlikely(count == 0)) {
        return s->prev[str & s->w_mask];
    }

    ret = 0;
    lp = str + count - 1; /* last position */

    for (p = str; p <= lp; p++) {
        unsigned hm;

        UPDATE_HASH_CRC_INTERNAL(s, hm, *(unsigned *)s->window[p]);

        if (s->head[hm] != p) {
            s->prev[p & s->w_mask] = s->head[hm];
            s->head[hm] = p;
            if (p == lp) {
                ret = s->prev[lp & s->w_mask];
            }
        }
    }
    return ret;
}
