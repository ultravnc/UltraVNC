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

project(libudtcloud LANGUAGES CXX)

set(CMAKE_CXX_FLAGS "-g -O0 -Wno-write-strings -Wno-conversion-null")



set(LIBUDTCLOUD_SOURCES
    proxy/Cloudthread.cpp
    proxy/proxy.cpp
    proxy/ringbuffer.cpp
)
add_library(libudtcloud STATIC)
target_include_directories(libudtcloud PRIVATE "${CMAKE_SOURCE_DIR}/../UdtCloudlib")
target_include_directories(libudtcloud PRIVATE "${CMAKE_SOURCE_DIR}/../UdtCloudlib/proxy")
foreach(file ${LIBUDTCLOUD_SOURCES})
    target_sources(libudtcloud PRIVATE "${CMAKE_SOURCE_DIR}/../UdtCloudlib/${file}")
endforeach(file)
target_link_libraries(libudtcloud
    libudt4
)
