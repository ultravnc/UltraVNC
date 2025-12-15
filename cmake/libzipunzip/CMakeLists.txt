# // This file is part of UltraVNC
# // https://github.com/ultravnc/UltraVNC
# // https://uvnc.com/
# //
# // SPDX-License-Identifier: GPL-3.0-or-later
# //
# // SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
# // SPDX-FileCopyrightText: Copyright (C) 1999-2002 Vdacc-VNC & eSVNC Projects. All Rights Reserved.
# //


project(libzipunzip LANGUAGES CXX RC)

set(${PROJECT_NAME}_SOURCES
    api.c
    crc32.c
    crctab.c
    crypt.c
    explode.c
    extract.c
    fileio.c
    globals.c
    inflate.c
    list.c
    match.c
    process.c
    unshrink.c
    win32/nt.c
    win32/win32.c
    windll/windll.c
    zipinfo.c
)
add_library(${PROJECT_NAME} STATIC)
foreach(file ${${PROJECT_NAME}_SOURCES})
    target_sources(${PROJECT_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/../zipunzip_src/unzip/${file}")
endforeach(file)
target_compile_definitions(${PROJECT_NAME} PRIVATE NDEBUG _WINDOWS WIN32 WINDLL DLL UNZIPLIB USE_EF_UT_TIME)
