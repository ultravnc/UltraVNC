#ifdef _MSC_VER
#pragma once
#endif

#include "zutil.h"
#include "deflate.h"

#include <assert.h>
#include <inttypes.h>

#ifndef _MSC_VER
#define UPDATE_HASH_CRC_INTERNAL(s,h,c) ( \
{\
    deflate_state *z_const state = (s);\
    unsigned val = (c); \
    unsigned hash = 0;\
    if (state->level >= 6) val &= 0xFFFFFF; \
    __asm__ __volatile__ ("crc32 %1,%0\n\t" : "+r" (hash) : "r" (val) : ); \
    h = hash & s->hash_mask; \
})
#else
#include <intrin.h>

#define UPDATE_HASH_CRC_INTERNAL(s, h, c) \
	(h = _mm_crc32_u32(0, \
		((deflate_state * z_const)s)->level >= 6 \
		 ?  (c) & 0xFFFFFF \
		 :   (c)) \
	 & ((deflate_state * z_const)s)->hash_mask)
#endif

#define UPDATE_HASH(s,h,c) ( \
    x86_cpu_has_sse42 ? UPDATE_HASH_CRC_INTERNAL(s, h, *(unsigned *)((uintptr_t)(&c) - (MIN_MATCH-1))) : UPDATE_HASH_C(s,h,c)\
) 


/* Safe to inline this as GCC/clang will use inline asm and Visual Studio will
* use intrinsic without extra params
*/
INLINE Pos insert_string_sse(deflate_state *const s, const Pos str)
{
    Pos ret;
    unsigned hm;

    UPDATE_HASH_CRC_INTERNAL(s, hm, *(unsigned *)&s->window[str]);

    ret = s->head[hm];
    s->head[hm] = str;
    s->prev[str & s->w_mask] = ret;
    return ret;
}
