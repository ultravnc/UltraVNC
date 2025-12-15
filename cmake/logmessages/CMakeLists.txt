# // This file is part of UltraVNC
# // https://github.com/ultravnc/UltraVNC
# // https://uvnc.com/
# //
# // SPDX-License-Identifier: GPL-3.0-or-later
# //
# // SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
# // SPDX-FileCopyrightText: Copyright (C) 1999-2002 Vdacc-VNC & eSVNC Projects. All Rights Reserved.
# //


project(logmessages LANGUAGES CXX RC)

if("${CMAKE_MC_COMPILER}" STREQUAL "" AND NOT "${CMAKE_RC_COMPILER}" STREQUAL "")
    string(REGEX REPLACE "windres$" "windmc" CMAKE_MC_COMPILER ${CMAKE_RC_COMPILER})
    string(REGEX REPLACE "rc.exe$" "mc.exe" CMAKE_MC_COMPILER ${CMAKE_MC_COMPILER})
endif()
message(STATUS "Found resource compiler: ${CMAKE_RC_COMPILER}")
message(STATUS "Found message compiler: ${CMAKE_MC_COMPILER}")

set(${PROJECT_NAME}_MESSAGES
    messages.mc
)
add_library(${PROJECT_NAME} SHARED)
foreach(file ${${PROJECT_NAME}_MESSAGES})
    set(file "${CMAKE_SOURCE_DIR}/../addon/eventMessageLogger/${file}")
    target_sources(${PROJECT_NAME} PRIVATE "${file}")
    get_filename_component(header ${file} NAME)
    string(REGEX REPLACE "\.mc$" ".h" header ${header})
    set_source_files_properties(${header} PROPERTIES GENERATED TRUE)
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/MSG00001.bin
        BYPRODUCTS ${CMAKE_CURRENT_BINARY_DIR}/${header}
        DEPENDS ${file}
        COMMAND ${CMAKE_MC_COMPILER}
        ARGS
        -h ${CMAKE_CURRENT_BINARY_DIR}
        -r ${CMAKE_CURRENT_BINARY_DIR}
        ${file}
        VERBATIM
    )
    target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
endforeach(file)
target_sources(${PROJECT_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/../addon/eventMessageLogger/messages.rc")
set_source_files_properties(${CMAKE_SOURCE_DIR}/../addon/eventMessageLogger/messages.rc LANGUAGE RC)
set_source_files_properties(${CMAKE_SOURCE_DIR}/../addon/eventMessageLogger/messages.rc OBJECT_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/MSG00001.bin)
target_compile_definitions(${PROJECT_NAME} PRIVATE WIN32;NDEBUG;_WINDOWS;_USRDLL;LOGMESSAGES_EXPORTS)
set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")
set_target_properties(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE "CXX" )
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC" OR "${CMAKE_CXX_COMPILER_FRONTEND_VARIANT}" STREQUAL "MSVC")
    target_link_options(${PROJECT_NAME} PRIVATE "/NOENTRY")
endif()
install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${CMAKE_BINARY_DIR}/ultravnc)
if (MSVC)
    install(FILES $<TARGET_PDB_FILE:${PROJECT_NAME}> DESTINATION ${CMAKE_BINARY_DIR}/ultravnc OPTIONAL)
endif()
