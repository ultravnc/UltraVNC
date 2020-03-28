#ifdef _MSC_VER
#pragma once
#endif

#ifndef __AARCH64__
#define __AARCH64__

#include "deflate.h" // for deflate_state

// Depending on the compiler flavor, size_t may be defined in one or the other header. See:
// http://stackoverflow.com/questions/26410466/gcc-linaro-compiler-throws-error-unknown-type-name-size-t
#include <stdint.h>
#include <stddef.h>

uint32_t adler32_neon(uint32_t adler, const unsigned char *buf, size_t len);
uint32_t crc32_acle(uint32_t crc, const unsigned char *buf, uint64_t len);

void fill_window_arm(deflate_state *s);
Pos insert_string_acle(deflate_state *const s, const Pos str, unsigned int count);

#endif
