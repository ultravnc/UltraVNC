cmake_minimum_required(VERSION 3.1.0)

project(libjpeg LANGUAGES C)

set(CMAKE_C_FLAGS "-g -O0 -Wno-write-strings")



set(LIBZSTD_SOURCES
    common/entropy_common.c
    common/error_private.c
    common/fse_decompress.c
    common/xxhash.c
    common/zstd_common.c
    compress/fse_compress.c
    compress/hist.c
    compress/huf_compress.c
    compress/zstd_compress.c
    compress/zstd_compress_literals.c
    compress/zstd_compress_sequences.c
    compress/zstd_compress_superblock.c
    compress/zstd_double_fast.c
    compress/zstd_fast.c
    compress/zstd_lazy.c
    compress/zstd_ldm.c
    compress/zstd_opt.c
    decompress/huf_decompress.c
    decompress/zstd_ddict.c
    decompress/zstd_decompress.c
    decompress/zstd_decompress_block.c
)
add_library(libzstd STATIC)
foreach(file ${LIBZSTD_SOURCES})
    target_sources(libzstd PRIVATE "${CMAKE_SOURCE_DIR}/../zstd/lib/${file}")
endforeach(file)
