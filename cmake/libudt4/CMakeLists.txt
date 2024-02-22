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

project(libudt4 LANGUAGES CXX)

set(CMAKE_CXX_FLAGS "-g -O0 -Wno-write-strings -Wno-conversion-null")



set(LIBUDT4_SOURCES
    src/api.cpp
    src/buffer.cpp
    src/cache.cpp
    src/ccc.cpp
    src/channel.cpp
    src/common.cpp
    src/core.cpp
    src/epoll.cpp
    src/list.cpp
    src/md5.cpp
    src/packet.cpp
    src/queue.cpp
    src/window.cpp
)
add_library(libudt4 STATIC)
target_include_directories(libudt4 PRIVATE "${CMAKE_SOURCE_DIR}/../udt4")
foreach(file ${LIBUDT4_SOURCES})
    target_sources(libudt4 PRIVATE "${CMAKE_SOURCE_DIR}/../udt4/${file}")
endforeach(file)
target_link_libraries(libudt4
    ws2_32
    wsock32
)
