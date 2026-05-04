@echo off
echo Building UDT STUN Bridge...

set UDT_PATH=..\..\CloudConnect\CloudConnectWin\udt4\src

cl.exe /EHsc /std:c++17 /MDd /I%UDT_PATH% stun_bridge_main.cpp /link %UDT_PATH%\udt.lib ws2_32.lib /out:udt_stun_bridge.exe

if %ERRORLEVEL% == 0 (
    echo.
    echo ✅ Build successful: udt_stun_bridge.exe
    echo.
    echo Usage:
    echo   Server: udt_stun_bridge server
    echo   Client: udt_stun_bridge client ^<server_ip^> ^<server_port^>
) else (
    echo ❌ Build failed
)
