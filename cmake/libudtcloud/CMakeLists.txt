# // This file is part of UltraVNC
# // https://github.com/ultravnc/UltraVNC
# // https://uvnc.com/
# //
# // SPDX-License-Identifier: GPL-3.0-or-later
# //
# // SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
# // SPDX-FileCopyrightText: Copyright (C) 1999-2002 Vdacc-VNC & eSVNC Projects. All Rights Reserved.
# //


project(libudtcloud LANGUAGES CXX RC)

set(${PROJECT_NAME}_SOURCES
    proxy/Cloudthread.cpp
    proxy/proxy.cpp
    proxy/ringbuffer.cpp
)
add_library(${PROJECT_NAME} STATIC)
target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/../UdtCloudlib")
target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/../UdtCloudlib/proxy")
foreach(file ${${PROJECT_NAME}_SOURCES})
    target_sources(${PROJECT_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/../UdtCloudlib/${file}")
endforeach(file)
target_link_libraries(${PROJECT_NAME}
    libudt4
)
