/////////////////////////////////////////////////////////////////////////////
//  Copyright (C) 2002-2024 UltraVNC Team Members. All Rights Reserved.
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
//  USA.
//
//  If the source code for the program is not available from the place from
//  which you received this file, check
//  https://uvnc.com/
//
////////////////////////////////////////////////////////////////////////////


cmake_minimum_required(VERSION 3.1.0)

project(libjpeg LANGUAGES C ASM_NASM)

set(CMAKE_C_FLAGS "-g -O0 -Wno-write-strings")
set(CMAKE_CXX_FLAGS "-g -O0 -Wno-write-strings -Wno-conversion-null")



enable_language(ASM_NASM)

set(LIBJPEG_SOURCES
    jaricom.c
    jcapimin.c
    jcapistd.c
    jcarith.c
    jccoefct.c
    jccolor.c
    jcdctmgr.c
    jchuff.c
    jcinit.c
    jcmainct.c
    jcmarker.c
    jcmaster.c
    jcomapi.c
    jcparam.c
    jcphuff.c
    jcprepct.c
    jcsample.c
    jctrans.c
    jdapimin.c
    jdapistd.c
    jdarith.c
    jdatadst.c
    jdatasrc.c
    jdcoefct.c
    jdcolor.c
    jddctmgr.c
    jdhuff.c
    jdinput.c
    jdmainct.c
    jdmarker.c
    jdmaster.c
    jdmerge.c
    jdphuff.c
    jdpostct.c
    jdsample.c
    jdtrans.c
    jerror.c
    jfdctflt.c
    jfdctfst.c
    jfdctint.c
    jidctflt.c
    jidctfst.c
    jidctint.c
    jidctred.c
    jmemmgr.c
    jmemnobs.c
    jquant1.c
    jquant2.c
    jutils.c
)
add_library(libjpeg STATIC)
target_include_directories(libjpeg PRIVATE "${CMAKE_SOURCE_DIR}/../libjpeg-turbo-win/simd")
target_include_directories(libjpeg PRIVATE "${CMAKE_SOURCE_DIR}/../libjpeg-turbo-win/simd/nasm")
foreach(file ${LIBJPEG_SOURCES})
    target_sources(libjpeg PRIVATE "${CMAKE_SOURCE_DIR}/../libjpeg-turbo-win/${file}")
endforeach(file)

if( ${CMAKE_SYSTEM_PROCESSOR} STREQUAL "x86_64" )
    set(LIBJPEGSIMD_DIR "x86_64")
    set(CMAKE_ASM_NASM_FLAGS "-DWIN64 -D__x86_64__")
    target_include_directories(libjpeg PRIVATE "${CMAKE_SOURCE_DIR}/../libjpeg-turbo-win/simd/x86_64")
    set(LIBJPEGSIMD_SOURCES
        jccolor-avx2.asm
        jccolor-sse2.asm
        jcgray-avx2.asm
        jcgray-sse2.asm
        jchuff-sse2.asm
        jcphuff-sse2.asm
        jcsample-avx2.asm
        jcsample-sse2.asm
        jdcolor-avx2.asm
        jdcolor-sse2.asm
        jdmerge-avx2.asm
        jdmerge-sse2.asm
        jdsample-avx2.asm
        jdsample-sse2.asm
        jfdctflt-sse.asm
        jfdctfst-sse2.asm
        jfdctint-avx2.asm
        jfdctint-sse2.asm
        jidctflt-sse2.asm
        jidctfst-sse2.asm
        jidctint-avx2.asm
        jidctint-sse2.asm
        jidctred-sse2.asm
        jquantf-sse2.asm
        jquanti-avx2.asm
        jquanti-sse2.asm
        jsimd.c
        jsimdcpu.asm
    )
else()
    set(LIBJPEGSIMD_DIR "i386")
    set(CMAKE_ASM_NASM_FLAGS "-DWIN32 -DMSVC")
    target_include_directories(libjpeg PRIVATE "${CMAKE_SOURCE_DIR}/../libjpeg-turbo-win/simd/i386")
    set(LIBJPEGSIMD_SOURCES
        jccolor-avx2.asm
        jccolor-mmx.asm
        jccolor-sse2.asm
        jcgray-avx2.asm
        jcgray-mmx.asm
        jcgray-sse2.asm
        jchuff-sse2.asm
        jcphuff-sse2.asm
        jcsample-avx2.asm
        jcsample-mmx.asm
        jcsample-sse2.asm
        jdcolor-avx2.asm
        jdcolor-mmx.asm
        jdcolor-sse2.asm
        jdmerge-avx2.asm
        jdmerge-mmx.asm
        jdmerge-sse2.asm
        jdsample-avx2.asm
        jdsample-mmx.asm
        jdsample-sse2.asm
        jfdctflt-3dn.asm
        jfdctflt-sse.asm
        jfdctfst-mmx.asm
        jfdctfst-sse2.asm
        jfdctint-avx2.asm
        jfdctint-mmx.asm
        jfdctint-sse2.asm
        jidctflt-3dn.asm
        jidctflt-sse.asm
        jidctflt-sse2.asm
        jidctfst-mmx.asm
        jidctfst-sse2.asm
        jidctint-avx2.asm
        jidctint-mmx.asm
        jidctint-sse2.asm
        jidctred-mmx.asm
        jidctred-sse2.asm
        jquant-3dn.asm
        jquant-mmx.asm
        jquant-sse.asm
        jquantf-sse2.asm
        jquanti-avx2.asm
        jquanti-sse2.asm
        jsimd.c
        jsimdcpu.asm
    )
endif() #CMAKE_SYSTEM_PROCESSOR
foreach(file ${LIBJPEGSIMD_SOURCES})
    target_sources(libjpeg PRIVATE "${CMAKE_SOURCE_DIR}/../libjpeg-turbo-win/simd/${LIBJPEGSIMD_DIR}/${file}")
endforeach(file)
