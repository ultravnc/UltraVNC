# // This file is part of UltraVNC
# // https://github.com/ultravnc/UltraVNC
# // https://uvnc.com/
# //
# // SPDX-License-Identifier: GPL-3.0-or-later
# //
# // SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
# // SPDX-FileCopyrightText: Copyright (C) 1999-2002 Vdacc-VNC & eSVNC Projects. All Rights Reserved.
# //


project(SecureVNCPlugin LANGUAGES CXX RC)

find_package(OpenSSL REQUIRED)

set(${PROJECT_NAME}_SOURCES
    Base64.cpp
    CryptUtils.cpp
    Dialogs.cpp
    dllmain.cpp
    IntegratedSecureVNCPluginObject.cpp
    MultithreadedPlugin.cpp
    SecureVNCPlugin.cpp
    SecureVNCPluginObject.cpp
    stdafx.cpp
    Utils.cpp
)
add_library(${PROJECT_NAME} SHARED)
foreach(file ${${PROJECT_NAME}_SOURCES})
    target_sources(${PROJECT_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/../DSMPlugin/SecureVNC/SecureVNCPlugin/${file}")
endforeach(file)
target_sources(${PROJECT_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/../DSMPlugin/SecureVNC/SecureVNCPlugin/SecureVNCPlugin.rc")
set_source_files_properties(${CMAKE_SOURCE_DIR}/../DSMPlugin/SecureVNC/SecureVNCPlugin/SecureVNCPlugin.rc LANGUAGE RC)
target_compile_definitions(${PROJECT_NAME} PRIVATE WIN32 NDEBUG _WINDOWS _USRDLL SECUREVNCPLUGIN_EXPORTS)
set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")
target_link_libraries(${PROJECT_NAME} PRIVATE OpenSSL::SSL)
target_link_libraries(${PROJECT_NAME} PRIVATE OpenSSL::Crypto)
install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${CMAKE_BINARY_DIR}/ultravnc)
if (MSVC)
    install(FILES $<TARGET_PDB_FILE:${PROJECT_NAME}> DESTINATION ${CMAKE_BINARY_DIR}/ultravnc OPTIONAL)
endif()
