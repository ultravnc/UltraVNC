project(vncviewer LANGUAGES C CXX RC)

find_package(JPEG REQUIRED)
find_package(libjpeg-turbo CONFIG REQUIRED)
find_package(ZLIB REQUIRED)
find_package(zstd CONFIG REQUIRED)

set(VNCVIEWER_SOURCES
    ../DSMPlugin/DSMPlugin.cpp
    ../ZipUnZip32/ZipUnzip32.cpp
    ../common/Clipboard.cpp
    ../common/Hyperlinks.cpp
    ../common/UltraVncZ.cpp
    ../common/mnemonic.c
    ../common/mn_wordlist.c
    ../common/d3des.c
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
    ClientConnectionRSAAES.cpp
    ClientConnectionRaw.cpp
    ClientConnectionTLS.cpp
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
    InfoBox.cpp
    SessionDialog.cpp
    SessionDialogTabs.cpp
    Snapshot.cpp
    TextChat.cpp
    UltraVNCHerperFunctions.cpp
    VNCOptions.cpp
    VNCviewerApp.cpp
    VNCviewerApp32.cpp
    buildtime.cpp
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
    target_compile_definitions(vncviewer PRIVATE _X64 _VIEWER _VCPKG)
else()
    target_compile_definitions(vncviewer PRIVATE _VIEWER _VCPKG)
endif()
target_link_libraries(vncviewer
    PRIVATE JPEG::JPEG
    PRIVATE $<IF:$<TARGET_EXISTS:libjpeg-turbo::turbojpeg>,libjpeg-turbo::turbojpeg,libjpeg-turbo::turbojpeg-static>
    PRIVATE ZLIB::ZLIB
    PRIVATE zstd::libzstd
    libomnithread
    librdr
    libudtcloud
    libzip32
    libzipunzip
    comctl32
    comdlg32
    crypt32
    cryptui
    gdi32
    gdiplus
    imm32
    secur32
    shlwapi
    version
    winmm
    ws2_32
)
