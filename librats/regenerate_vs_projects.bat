@echo off
REM Regenerate Visual Studio project files for both Win32 and x64

cd /d "%~dp0"

if not exist win32 mkdir win32
cd win32

echo Regenerating CMake project files for Visual Studio...
cmake .. -G "Visual Studio 17 2022" -T v142 -DRATS_BUILD_TESTS=OFF -DRATS_BUILD_EXAMPLES=ON -DRATS_STATIC_LIBRARY=ON

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ERROR: CMake generation failed!
    echo.
    echo If cmake is not found, you have two options:
    echo   1. Install CMake from https://cmake.org/download/
    echo   2. Use the pre-generated project files if they exist
    echo.
    pause
    exit /b 1
)

echo.
echo CMake project files regenerated successfully!
echo You can now build for both Win32 and x64 platforms.
echo.
pause
