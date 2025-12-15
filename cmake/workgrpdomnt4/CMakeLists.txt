# // This file is part of UltraVNC
# // https://github.com/ultravnc/UltraVNC
# // https://uvnc.com/
# //
# // SPDX-License-Identifier: GPL-3.0-or-later
# //
# // SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
# // SPDX-FileCopyrightText: Copyright (C) 1999-2002 Vdacc-VNC & eSVNC Projects. All Rights Reserved.
# //


project(workgrpdomnt4 LANGUAGES CXX RC)

set(${PROJECT_NAME}_SOURCES
    workgrpdomnt4.cpp
)
add_library(${PROJECT_NAME} SHARED)
foreach(file ${${PROJECT_NAME}_SOURCES})
    target_sources(${PROJECT_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/../addon/ms-logon/workgrpdomnt4/${file}")
endforeach(file)
target_sources(${PROJECT_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/../addon/ms-logon/workgrpdomnt4/workgrpdomnt4.rc")
set_source_files_properties(${CMAKE_SOURCE_DIR}/../addon/ms-logon/workgrpdomnt4/workgrpdomnt4.rc LANGUAGE RC)
target_compile_definitions(${PROJECT_NAME} PRIVATE WIN32 NDEBUG _WINDOWS WORKGRPDOMNT4_EXPORTS)
if (MINGW)
    target_link_options(${PROJECT_NAME} PRIVATE "-municode")
endif()
set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")
install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${CMAKE_BINARY_DIR}/ultravnc)
if (MSVC)
    install(FILES $<TARGET_PDB_FILE:${PROJECT_NAME}> DESTINATION ${CMAKE_BINARY_DIR}/ultravnc OPTIONAL)
endif()
