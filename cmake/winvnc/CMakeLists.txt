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

project(winvnc LANGUAGES C CXX RC)

set(CMAKE_C_FLAGS "-g -O0 -Wno-write-strings")
set(CMAKE_CXX_FLAGS "-g -O0 -Wno-write-strings -Wno-conversion-null")



include(../CMakeLists_libomnithread.txt)

include(../CMakeLists_librdr.txt)

include(../CMakeLists_libuvncsettings.txt)

include(../CMakeLists_libz.txt)

include(../CMakeLists_libzip32.txt)

include(../CMakeLists_libzipunzip.txt)



set(WINVNC_SOURCES
    ../../DSMPlugin/DSMPlugin.cpp
    ../../ZipUnZip32/ZipUnzip32.cpp
    ../../common/buildtime.cpp
    ../../common/Clipboard.cpp
    ../../common/d3des.c
    ../../common/UltraVncZ.cpp
    ../../common/win32_helpers.cpp
    ../../lzo/minilzo.c
    ../../rfb/dh.cpp
    CloudDialog.cpp
    CpuUsage.cpp
    DeskdupEngine.cpp
    Dtwinver.cpp
    HelperFunctions.cpp
    HideDesktop.cpp
    IPC.cpp
    LayeredWindows.cpp
    MouseSimulator.cpp
    ScreenCapture.cpp
    SettingsManager.cpp
    TextChat.cpp
    UltraVNCService.cpp
    VirtualDisplay.cpp
    benchmark.cpp
    cadthread.cpp
    credentials.cpp
    getinfo.cpp
    inifile.cpp
    initipp.cpp
    read_write_ini.cpp
    rfbRegion_X11.cxx
    rfbRegion_win32.cpp
    rfbUpdateTracker.cpp
    service.cpp
    stdhdrs.cpp
    translate.cpp
    uvncUiAccess.cpp
    videodriver.cpp
    videodrivercheck.cpp
    vistahook.cpp
    vncDesktopSW.cpp
    vncEncodeTight.cpp
    vncEncodeUltra.cpp
    vncEncodeUltra2.cpp
    vncEncodeXZ.cpp
    vncEncodeZlib.cpp
    vncEncodeZlibHex.cpp
    vncListDlg.cpp
    vncMultiMonitor.cpp
    vncOSVersion.cpp
    vncabout.cpp
    vncacceptdialog.cpp
    vncauth.c
    vncbuffer.cpp
    vncclient.cpp
    vncconndialog.cpp
    vncdesktop.cpp
    vncdesktopsink.cpp
    vncdesktopthread.cpp
    vncencodecorre.cpp
    vncencodehext.cpp
    vncencoder.cpp
    vncencoderCursor.cpp
    vncencoderre.cpp
    vncencodezrle.cpp
    vnchttpconnect.cpp
    vncinsthandler.cpp
    vnckeymap.cpp
    vnclog.cpp
    vncmenu.cpp
    vncntlm.cpp
    vncproperties.cpp
    vncpropertiesPoll.cpp
    vncserver.cpp
    vncsetauth.cpp
    vncsockconnect.cpp
    vnctimedmsgbox.cpp
    vsocket.cpp
    winvnc.cpp
)
add_executable(winvnc)
target_include_directories(winvnc PRIVATE "${CMAKE_SOURCE_DIR}/..")
target_include_directories(winvnc PRIVATE "${CMAKE_SOURCE_DIR}/../common")
target_include_directories(winvnc PRIVATE "${CMAKE_SOURCE_DIR}/../winvnc")
target_include_directories(winvnc PRIVATE "${CMAKE_SOURCE_DIR}/../winvnc/omnithread")
foreach(file ${WINVNC_SOURCES})
    target_sources(winvnc PRIVATE "${CMAKE_SOURCE_DIR}/../winvnc/winvnc/${file}")
endforeach(file)
target_sources(winvnc PRIVATE "${CMAKE_SOURCE_DIR}/../winvnc/winvnc/winvnc.rc")
set_source_files_properties(${CMAKE_SOURCE_DIR}/../winvnc/winvnc/winvnc.rc LANGUAGE RC)
set_source_files_properties(${CMAKE_SOURCE_DIR}/../winvnc/winvnc/winvnc.rc PROPERTIES COMPILE_FLAGS "-Ocoff")
if( ${CMAKE_SYSTEM_PROCESSOR} STREQUAL "x86_64" )
    target_compile_definitions(winvnc PRIVATE _X64 _WIN32_WINNT=0x601)
else()
    target_compile_definitions(winvnc PRIVATE _WIN32_WINNT=0x601)
endif()
target_link_libraries(winvnc
    libjpeg
    libomnithread_${PROJECT_NAME}
    librdr_${PROJECT_NAME}
    libuvncsettings_${PROJECT_NAME}
    libz_${PROJECT_NAME}
    libzip32_${PROJECT_NAME}
    libzipunzip_${PROJECT_NAME}
    libzstd
    libudtcloud
    comctl32
    comdlg32
    dwmapi
    imm32
    iphlpapi
    shlwapi
    userenv
    version
    ws2_32
)
