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

project(uvnc_settings LANGUAGES C CXX RC)

set(CMAKE_C_FLAGS "-g -O0 -Wno-write-strings")
set(CMAKE_CXX_FLAGS "-g -O0 -Wno-write-strings -Wno-conversion-null")



set(VNCVIEWER_SOURCES
    capture.cpp
    connections.cpp
    createsfx2.cpp
    createsfx3.cpp
    dsmplugin.cpp
    filetransfer.cpp
    firewall.cpp
    inifile.cpp
    log.cpp
    misc.cpp
    network.cpp
    security.cpp
    service.cpp
    stdafx.cpp
    upnp.cpp
    uvnc_settings.cpp
    videodrivercheck.cpp
    vncOSVersion.cpp
    vncsetauth.cpp
)
add_executable(uvnc_settings)
target_include_directories(uvnc_settings PRIVATE "${CMAKE_SOURCE_DIR}/../DSMPlugin")
target_include_directories(uvnc_settings PRIVATE "${CMAKE_SOURCE_DIR}/../vncviewer/omnithread")
foreach(file ${VNCVIEWER_SOURCES})
    target_sources(uvnc_settings PRIVATE "${CMAKE_SOURCE_DIR}/../uvnc_settings/uvnc_settings/${file}")
endforeach(file)
target_sources(uvnc_settings PRIVATE "${CMAKE_SOURCE_DIR}/../uvnc_settings/uvnc_settings/uvnc_settings.rc")
set_source_files_properties(${CMAKE_SOURCE_DIR}/../uvnc_settings/uvnc_settings/uvnc_settings.rc LANGUAGE RC)
set_source_files_properties(${CMAKE_SOURCE_DIR}/../uvnc_settings/uvnc_settings/uvnc_settings.rc PROPERTIES COMPILE_FLAGS "-Ocoff")
if( ${CMAKE_SYSTEM_PROCESSOR} STREQUAL "x86_64" )
    target_compile_definitions(uvnc_settings PRIVATE _X64)
endif()
target_link_libraries(uvnc_settings
    libomnithread_vncviewer
    comctl32
    version
)
