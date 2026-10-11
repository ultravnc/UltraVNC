@echo off
rem This file is part of UltraVNC
rem https://github.com/ultravnc/UltraVNC
rem https://uvnc.com/
rem
rem SPDX-License-Identifier: GPL-3.0-or-later
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
rc /fo TokenValidator.res TokenValidator.rc
cl.exe -LD -FeTokenValidator.dll TokenValidator.cpp TokenCore.cpp ..\3rdparty\qrcodegen\qrcodegen.cpp TokenValidator.res advapi32.lib bcrypt.lib crypt32.lib user32.lib gdi32.lib /EHsc
if errorlevel 1 exit /b 1
cl.exe -FeTokenSetup.exe TokenSetup.cpp TokenCore.cpp ..\3rdparty\qrcodegen\qrcodegen.cpp advapi32.lib bcrypt.lib crypt32.lib user32.lib gdi32.lib /EHsc /link /MANIFEST:EMBED /MANIFESTINPUT:TokenSetup.manifest
if errorlevel 1 exit /b 1

copy /Y TokenValidator.dll C:\uvnc\64\xp\
call C:\uvnc\sign.bat "C:\uvnc\64\xp\TokenValidator.dll"
copy /Y TokenSetup.exe C:\uvnc\64\xp\
call C:\uvnc\sign.bat "C:\uvnc\64\xp\TokenSetup.exe"
