# // This file is part of UltraVNC
# // https://github.com/ultravnc/UltraVNC
# // https://uvnc.com/
# //
# // SPDX-License-Identifier: GPL-3.0-or-later
# //
# // SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
# // SPDX-FileCopyrightText: Copyright (C) 1999-2002 Vdacc-VNC & eSVNC Projects. All Rights Reserved.
# //


project(libzip32 LANGUAGES CXX RC)

set(${PROJECT_NAME}_SOURCES
    api.c
    crypt.c
    deflate.c
    fileio.c
    globals.c
    trees.c
    util.c
    win32/nt.c
    win32/win32.c
    win32/win32zip.c
    windll/windll.c
    zip.c
    zipfile.c
    zipup.c
)
add_library(${PROJECT_NAME} STATIC)
foreach(file ${${PROJECT_NAME}_SOURCES})
    target_sources(${PROJECT_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/../zipunzip_src/zip20/${file}")
endforeach(file)
target_compile_definitions(${PROJECT_NAME} PRIVATE NDEBUG _WINDOWS WIN32 NO_ASM WINDLL MSDOS USE_ZIPMAIN ZIPLIB)
