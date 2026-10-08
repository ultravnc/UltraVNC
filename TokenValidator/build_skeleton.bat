@echo off
if exist "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat" (
    call "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat"
) else if exist "C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\VC\Auxiliary\Build\vcvars64.bat" (
    call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\VC\Auxiliary\Build\vcvars64.bat"
) else (
    echo No supported Visual Studio environment found.
    exit /b 1
)
cl.exe -LD -FeCustomValidatorSkeleton.dll CustomValidatorSkeleton.cpp advapi32.lib bcrypt.lib crypt32.lib user32.lib gdi32.lib /EHsc
