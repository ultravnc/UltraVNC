# // This file is part of UltraVNC
# // https://github.com/ultravnc/UltraVNC
# // https://uvnc.com/
# //
# // SPDX-License-Identifier: GPL-3.0-or-later
# //
# // SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
# // SPDX-FileCopyrightText: Copyright (C) 1999-2002 Vdacc-VNC & eSVNC Projects. All Rights Reserved.
# //


project(libudt4 LANGUAGES CXX RC)

set(${PROJECT_NAME}_SOURCES
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
add_library(${PROJECT_NAME} STATIC)
target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/../udt4")
foreach(file ${${PROJECT_NAME}_SOURCES})
    target_sources(${PROJECT_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/../udt4/${file}")
endforeach(file)
target_link_libraries(${PROJECT_NAME}
    ws2_32
    wsock32
)
