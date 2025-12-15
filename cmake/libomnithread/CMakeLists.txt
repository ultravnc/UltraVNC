# // This file is part of UltraVNC
# // https://github.com/ultravnc/UltraVNC
# // https://uvnc.com/
# //
# // SPDX-License-Identifier: GPL-3.0-or-later
# //
# // SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
# // SPDX-FileCopyrightText: Copyright (C) 1999-2002 Vdacc-VNC & eSVNC Projects. All Rights Reserved.
# //


project(libomnithread LANGUAGES CXX RC)

set(${PROJECT_NAME}_SOURCES
    nt.cpp
)
add_library(${PROJECT_NAME} STATIC)
foreach(file ${${PROJECT_NAME}_SOURCES})
    target_sources(${PROJECT_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/../omnithread/${file}")
endforeach(file)
target_compile_definitions(${PROJECT_NAME} PRIVATE NDEBUG WIN32 _WINDOWS __NT__ _WINSTATIC __WIN32__ _WIN32_WINNT=0x400)
