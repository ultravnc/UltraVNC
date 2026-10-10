@echo off
rem This file is part of UltraVNC
rem https://github.com/ultravnc/UltraVNC
rem https://uvnc.com/
rem
rem SPDX-License-Identifier: MIT
rem
rem SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
rem
if exist "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat" (
    call "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat"
) else if exist "C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\VC\Auxiliary\Build\vcvars64.bat" (
    call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\VC\Auxiliary\Build\vcvars64.bat"
) else (
    echo No supported Visual Studio environment found.
    exit /b 1
)
cl.exe -LD -FeCustomValidatorSkeleton.dll CustomValidatorSkeleton.cpp advapi32.lib bcrypt.lib crypt32.lib user32.lib gdi32.lib /EHsc
