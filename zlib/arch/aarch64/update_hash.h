#ifdef _MSC_VER
#pragma once
#endif

#ifndef _MSC_VER
#include <arm_acle.h> // for __crc32w
#endif

#define UPDATE_HASH_CRC_INTERNAL(s, h, c) \
	(h = __crc32w(0, \
		((deflate_state *)s)->level >= 6 \
		 ?  (c) & 0xFFFFFF \
		 :  (c)) \
	 & ((deflate_state *)s)->hash_mask)

#define UPDATE_HASH(s, h, c) \
    UPDATE_HASH_CRC_INTERNAL(s, h, *(unsigned *)((uintptr_t)(&c) - (MIN_MATCH-1)))
