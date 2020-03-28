# zlib
Drop in replacement for zlib 1.2.11 with optimizations from various sources.

This fork is based on the official zlib repository:
https://github.com/madler/zlib

## 3rd Party Patches
- Optimizations from Intel without the new deflate strategies (quick, medium)  
  crc32: crc32 implementation with PCLMULQDQ optimized folding  
  deflate: slide_hash_sse in fill_window  
  deflate: use crc32 (SIMD) to calculate hash  
  https://github.com/jtkukunas/zlib

- Optimizations from Cloudflare   
  deflate: longest_match optimizations (https://github.com/cloudflare/zlib/commit/31043308c3d3edfb487d2c4cbe7290bd5b63c65c)  
  https://github.com/cloudflare/zlib

- Optimized longest_match  
  https://github.com/gildor2/fast_zlib  
  Adapted function to use crc32 (SIMD) to calculate hash and integrated match compare optimization from above  

- Other small changes  
  put_short optimization (https://github.com/Dead2/zlib-ng/commit/666581bbc17eaea62acb47cf47ab35a037e9b9d0)  
  https://github.com/Dead2/zlib-ng

- Optimizations for ARM  
  adler32: Adenilson Cavalcanti &lt;adenilson.cavalcanti@arm.com\>  
  fill_window: Mika T. Lindqvist &lt;postmaster@raasu.org\>  

- adler32-simd from Chromium  
  https://github.com/chromium/chromium/blob/master/third_party/zlib/adler32_simd.c  

## Additional changes
- Support and optimizations for MSVC15 compiler  
  Support for _M_ARM64    
  Use __forceinline

- Use tzcnt instead of bsf  
  This improves performance for AMD CPUs

- Implementation optimized for modern CPUs (Intel Nehalem)  
  Removed alignment loop in crc32  
  Adds temporary in crc32_little calcuation  
  Less manual unrolling

- Others  
  Optimized insert_string loop

## New features
- General purpose crc32 interface  
  Based on Intel's PCLMULQDQ crc32 implementation.  
  New functions:  
  crc32_init  
  crc32_update  
  crc32_final  
  Brings ~200% performance improvement over the original zlib crc32 implementation

## Performance
The performance results and pre-built binaries can be found here: https://github.com/matbech/zlib-perf