@echo off
rem This file is part of UltraVNC
rem https://github.com/ultravnc/UltraVNC
rem https://uvnc.com/
rem
rem SPDX-License-Identifier: GPL-3.0-or-later
rem
rem SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
rem
if exist "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars32.bat" (
    call "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars32.bat"
) else if exist "C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\VC\Auxiliary\Build\vcvars32.bat" (
    call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\VC\Auxiliary\Build\vcvars32.bat"
) else (
    echo No supported Visual Studio environment found.
    exit /b 1
)
rc /fo TokenValidator32.res TokenValidator.rc
cl.exe -LD -FeTokenValidator32.dll TokenValidator.cpp TokenCore.cpp ..\3rdparty\qrcodegen\qrcodegen.cpp TokenValidator32.res advapi32.lib bcrypt.lib crypt32.lib user32.lib gdi32.lib /EHsc
if errorlevel 1 exit /b 1

copy /Y TokenValidator32.dll C:\uvnc\32\xp\
call C:\uvnc\sign.bat "C:\uvnc\32\xp\TokenValidator32.dll"
