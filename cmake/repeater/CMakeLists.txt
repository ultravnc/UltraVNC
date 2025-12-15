# // This file is part of UltraVNC
# // https://github.com/ultravnc/UltraVNC
# // https://uvnc.com/
# //
# // SPDX-License-Identifier: GPL-3.0-or-later
# //
# // SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
# // SPDX-FileCopyrightText: Copyright (C) 1999-2002 Vdacc-VNC & eSVNC Projects. All Rights Reserved.
# //


project(repeater LANGUAGES CXX RC)

set(${PROJECT_NAME}_SOURCES
    gui.cpp
    lists_functions.cpp
    mode12_listener.cpp
    mode2_listener_server.cpp
    repeater.cpp
    socket_functions.cpp
    webgui/settings.c
    webgui/webclib.c
    webgui/webfs.c
    webgui/webio.c
    webgui/webobjs.c
    webgui/websys.c
    webgui/webtest.c
    webgui/webutils.c
    webgui/wsfdata.c
)
add_executable(${PROJECT_NAME} WIN32)
foreach(file ${${PROJECT_NAME}_SOURCES})
    target_sources(${PROJECT_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/../repeater/${file}")
endforeach(file)
target_sources(${PROJECT_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/../repeater/resources.rc")
set_source_files_properties(${CMAKE_SOURCE_DIR}/../repeater/resources.rc LANGUAGE RC)
target_link_libraries(${PROJECT_NAME}
    winmm
    ws2_32
)
install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${CMAKE_BINARY_DIR}/ultravnc)
if (MSVC)
    install(FILES $<TARGET_PDB_FILE:${PROJECT_NAME}> DESTINATION ${CMAKE_BINARY_DIR}/ultravnc OPTIONAL)
endif()
