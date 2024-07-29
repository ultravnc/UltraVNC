cmake_minimum_required(VERSION 3.1.0)

project(vncviewer LANGUAGES C CXX RC)

set(CMAKE_C_FLAGS "-g -O0 -Wno-write-strings")
set(CMAKE_CXX_FLAGS "-g -O0 -Wno-write-strings -Wno-conversion-null")



include(../CMakeLists_libz.txt)


set(VNCVIEWER_SOURCES
    ../DSMPlugin/DSMPlugin.cpp
    ../ZipUnZip32/ZipUnzip32.cpp
    ../common/Clipboard.cpp
    ../common/UltraVncZ.cpp
    ../common/win32_helpers.cpp
    ../lzo/minilzo.c
    ../rfb/dh.cpp
    AboutBox.cpp
    AccelKeys.cpp
    AuthDialog.cpp
    ClientConnection.cpp
    ClientConnectionCacheRect.cpp
    ClientConnectionClipboard.cpp
    ClientConnectionCoRRE.cpp
    ClientConnectionCopyRect.cpp
    ClientConnectionCursor.cpp
    ClientConnectionFile.cpp
    ClientConnectionFullScreen.cpp
    ClientConnectionHextile.cpp
    ClientConnectionRRE.cpp
    ClientConnectionRaw.cpp
    ClientConnectionTight.cpp
    ClientConnectionUltra.cpp
    ClientConnectionUltra2.cpp
    ClientConnectionZlib.cpp
    ClientConnectionZlibHex.cpp
    Daemon.cpp
    Exception.cpp
    FileTransfer.cpp
    FullScreenTitleBar.cpp
    KeyMap.cpp
    KeyMapjap.cpp
    Log.cpp
    LowLevelHook.cpp
    MRU.cpp
    MessBox.cpp
    MessBox2.cpp
    SessionDialog.cpp
    SessionDialogTabs.cpp
    Snapshot.cpp
    TextChat.cpp
    VNCOptions.cpp
    VNCviewerApp.cpp
    VNCviewerApp32.cpp
    buildtime.cpp
    d3des.c
    directx/directxviewer.cpp
    display.cpp
    sessiondialogLoadSave.cpp
    stdhdrs.cpp
    vncauth.c
    vnctouch.cpp
    vncviewer.cpp
    xz.cpp
    zrle.cpp
)
add_executable(vncviewer)
target_include_directories(vncviewer PRIVATE "${CMAKE_SOURCE_DIR}/..")
target_include_directories(vncviewer PRIVATE "${CMAKE_SOURCE_DIR}/../omnithread")
foreach(file ${VNCVIEWER_SOURCES})
    target_sources(vncviewer PRIVATE "${CMAKE_SOURCE_DIR}/../vncviewer/${file}")
endforeach(file)
target_sources(vncviewer PRIVATE "${CMAKE_SOURCE_DIR}/../vncviewer/res/vncviewer.rc")
set_source_files_properties(${CMAKE_SOURCE_DIR}/../vncviewer/res/vncviewer.rc LANGUAGE RC)
set_source_files_properties(${CMAKE_SOURCE_DIR}/../vncviewer/res/vncviewer.rc PROPERTIES COMPILE_FLAGS "-Ocoff")
if( ${CMAKE_SYSTEM_PROCESSOR} STREQUAL "x86_64" )
    target_compile_definitions(vncviewer PRIVATE _X64 _VIEWER)
else()
    target_compile_definitions(vncviewer PRIVATE _VIEWER)
endif()
target_link_libraries(vncviewer
    libjpeg
    libomnithread
    librdr
    libz_${PROJECT_NAME}
    libzstd
    libudtcloud
    libzip32
    libzipunzip
    comctl32
    comdlg32
    gdi32
    gdiplus
    imm32
    winmm
    ws2_32
)
