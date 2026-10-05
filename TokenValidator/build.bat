@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\VC\Auxiliary\Build\vcvars64.bat"
"C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\VC\Tools\MSVC\14.29.30133\bin\Hostx64\x64\cl.exe" -LD -FeTokenValidator.dll TokenValidator.cpp advapi32.lib bcrypt.lib crypt32.lib /EHsc
"C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\VC\Tools\MSVC\14.29.30133\bin\Hostx64\x64\cl.exe" -FeTokenSetup.exe TokenSetup.cpp qrcodegen.cpp advapi32.lib bcrypt.lib crypt32.lib /EHsc
