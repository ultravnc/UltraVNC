project(winvnc LANGUAGES C CXX RC)

find_package(JPEG REQUIRED)
find_package(libjpeg-turbo CONFIG REQUIRED)
find_package(ZLIB REQUIRED)
find_package(zstd CONFIG REQUIRED)

set(WINVNC_SOURCES
    ../../DSMPlugin/DSMPlugin.cpp
    ../../ZipUnZip32/ZipUnzip32.cpp
    ../../common/Clipboard.cpp
    ../../common/Hyperlinks.cpp
    ../../common/UltraVncZ.cpp
    ../../common/d3des.c
    ../../common/win32_helpers.cpp
    ../../lzo/minilzo.c
    ../../rfb/dh.cpp
    CloudDialog.cpp
    CpuUsage.cpp
    DeskdupEngine.cpp
    Dtwinver.cpp
    HelperFunctions.cpp
    HideDesktop.cpp
    IPC.cpp
    LayeredWindows.cpp
    MouseSimulator.cpp
    ScreenCapture.cpp
    SettingsManager.cpp
    TextChat.cpp
    UdpEchoServer.cpp
    UltraVNCService.cpp
    VirtualDisplay.cpp
    benchmark.cpp
    buildtime.cpp
    cadthread.cpp
    credentials.cpp
    getinfo.cpp
    inifile.cpp
    initipp.cpp
    read_write_ini.cpp
    rfbRegion_X11.cxx
    rfbRegion_win32.cpp
    rfbUpdateTracker.cpp
    service.cpp
    stdhdrs.cpp
    translate.cpp
    uvncUiAccess.cpp
    videodriver.cpp
    videodrivercheck.cpp
    vistahook.cpp
    vncDesktopSW.cpp
    vncEncodeTight.cpp
    vncEncodeUltra.cpp
    vncEncodeUltra2.cpp
    vncEncodeXZ.cpp
    vncEncodeZlib.cpp
    vncEncodeZlibHex.cpp
    vncListDlg.cpp
    vncMultiMonitor.cpp
    vncOSVersion.cpp
    vncabout.cpp
    vncacceptdialog.cpp
    vncauth.c
    vncbuffer.cpp
    vncclient.cpp
    vncconndialog.cpp
    vncdesktop.cpp
    vncdesktopsink.cpp
    vncdesktopthread.cpp
    vncencodecorre.cpp
    vncencodehext.cpp
    vncencoder.cpp
    vncencoderCursor.cpp
    vncencoderre.cpp
    vncencodezrle.cpp
    vnchttpconnect.cpp
    vncinsthandler.cpp
    vnckeymap.cpp
    vnclog.cpp
    vncmenu.cpp
    vncntlm.cpp
    vncproperties.cpp
    vncpropertiesPoll.cpp
    vncserver.cpp
    vncsetauth.cpp
    vncsockconnect.cpp
    vnctimedmsgbox.cpp
    vsocket.cpp
    winvnc.cpp
)
add_executable(winvnc)
target_include_directories(winvnc PRIVATE "${CMAKE_SOURCE_DIR}/..")
target_include_directories(winvnc PRIVATE "${CMAKE_SOURCE_DIR}/../common")
target_include_directories(winvnc PRIVATE "${CMAKE_SOURCE_DIR}/../omnithread")
target_include_directories(winvnc PRIVATE "${CMAKE_SOURCE_DIR}/../winvnc")
foreach(file ${WINVNC_SOURCES})
    target_sources(winvnc PRIVATE "${CMAKE_SOURCE_DIR}/../winvnc/winvnc/${file}")
endforeach(file)
target_sources(winvnc PRIVATE "${CMAKE_SOURCE_DIR}/../winvnc/winvnc/winvnc.rc")
set_source_files_properties(${CMAKE_SOURCE_DIR}/../winvnc/winvnc/winvnc.rc LANGUAGE RC)
set_source_files_properties(${CMAKE_SOURCE_DIR}/../winvnc/winvnc/winvnc.rc PROPERTIES COMPILE_FLAGS "-Ocoff")
if( ${CMAKE_SYSTEM_PROCESSOR} STREQUAL "x86_64" )
    target_compile_definitions(winvnc PRIVATE _X64 _WIN32_WINNT=0x601 _VCPKG)
else()
    target_compile_definitions(winvnc PRIVATE _WIN32_WINNT=0x601 _VCPKG)
endif()
target_link_libraries(winvnc
    PRIVATE JPEG::JPEG
    PRIVATE $<IF:$<TARGET_EXISTS:libjpeg-turbo::turbojpeg>,libjpeg-turbo::turbojpeg,libjpeg-turbo::turbojpeg-static>
    PRIVATE ZLIB::ZLIB
    PRIVATE zstd::libzstd
    libomnithread
    librdr
    libudtcloud
    libuvncsettings
    libzip32
    libzipunzip
    comctl32
    comdlg32
    dwmapi
    imm32
    iphlpapi
    shlwapi
    userenv
    version
    ws2_32
    wtsapi32
)
