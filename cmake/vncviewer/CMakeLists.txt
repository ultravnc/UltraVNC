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

project(vncviewer LANGUAGES C CXX RC)

set(CMAKE_C_FLAGS "-g -O0 -Wno-write-strings")
set(CMAKE_CXX_FLAGS "-g -O0 -Wno-write-strings -Wno-conversion-null")



include(../CMakeLists_libomnithread.txt)

include(../CMakeLists_librdr.txt)

include(../CMakeLists_libz.txt)

include(../CMakeLists_libzip32.txt)

include(../CMakeLists_libzipunzip.txt)



set(VNCVIEWER_SOURCES
    ../DSMPlugin/DSMPlugin.cpp
    ../ZipUnZip32/ZipUnzip32.cpp
    ../common/buildtime.cpp
    ../common/Clipboard.cpp
    ../common/d3des.c
    ../common/UltraVncZ.cpp
    ../common/win32_helpers.cpp
    ../lzo/minilzo.c
    ../rfb/dh.cpp
    AboutBox.cpp
    AccelKeys.cpp
    AuthDialog.cpp
    ClientConnection.cpp
    ClientConnectionCacheRect.cpp
    ClientConnectionClipboard.cpp
    ClientConnectionCoRRE.cpp
    ClientConnectionCopyRect.cpp
    ClientConnectionCursor.cpp
    ClientConnectionFile.cpp
    ClientConnectionFullScreen.cpp
    ClientConnectionHextile.cpp
    ClientConnectionRRE.cpp
    ClientConnectionRaw.cpp
    ClientConnectionTight.cpp
    ClientConnectionUltra.cpp
    ClientConnectionUltra2.cpp
    ClientConnectionZlib.cpp
    ClientConnectionZlibHex.cpp
    Daemon.cpp
    Exception.cpp
    FileTransfer.cpp
    FullScreenTitleBar.cpp
    KeyMap.cpp
    KeyMapjap.cpp
    Log.cpp
    LowLevelHook.cpp
    MRU.cpp
    MessBox.cpp
    MessBox2.cpp
    SessionDialog.cpp
    SessionDialogTabs.cpp
    Snapshot.cpp
    TextChat.cpp
    VNCOptions.cpp
    VNCviewerApp.cpp
    VNCviewerApp32.cpp
    directx/directxviewer.cpp
    display.cpp
    sessiondialogLoadSave.cpp
    stdhdrs.cpp
    vncauth.c
    vnctouch.cpp
    vncviewer.cpp
    xz.cpp
    zrle.cpp
)
add_executable(vncviewer)
target_include_directories(vncviewer PRIVATE "${CMAKE_SOURCE_DIR}/..")
target_include_directories(vncviewer PRIVATE "${CMAKE_SOURCE_DIR}/../vncviewer/omnithread")
foreach(file ${VNCVIEWER_SOURCES})
    target_sources(vncviewer PRIVATE "${CMAKE_SOURCE_DIR}/../vncviewer/${file}")
endforeach(file)
target_sources(vncviewer PRIVATE "${CMAKE_SOURCE_DIR}/../vncviewer/vncviewer.rc")
set_source_files_properties(${CMAKE_SOURCE_DIR}/../vncviewer/vncviewer.rc LANGUAGE RC)
set_source_files_properties(${CMAKE_SOURCE_DIR}/../vncviewer/vncviewer.rc PROPERTIES COMPILE_FLAGS "-Ocoff")
if( ${CMAKE_SYSTEM_PROCESSOR} STREQUAL "x86_64" )
    target_compile_definitions(vncviewer PRIVATE _X64 _VIEWER)
else()
    target_compile_definitions(vncviewer PRIVATE _VIEWER)
endif()
target_link_libraries(vncviewer
    libjpeg
    libomnithread_${PROJECT_NAME}
    librdr_${PROJECT_NAME}
    libz_${PROJECT_NAME}
    libzip32_${PROJECT_NAME}
    libzipunzip_${PROJECT_NAME}
    libzstd
    libudtcloud
    comctl32
    comdlg32
    gdi32
    gdiplus
    imm32
    winmm
    ws2_32
)
