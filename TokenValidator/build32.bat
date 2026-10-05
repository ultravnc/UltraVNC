@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\VC\Auxiliary\Build\vcvars32.bat"
cl.exe -LD -FeTokenValidator32.dll TokenValidator.cpp advapi32.lib bcrypt.lib crypt32.lib /EHsc
