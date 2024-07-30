
set(LIBUVNCSETTINGS_SOURCES
    capture.cpp
    connections.cpp
    filetransfer.cpp
    firewall.cpp
    log.cpp
    misc.cpp
    network.cpp
    security.cpp
    service.cpp
)
add_library(libuvncsettings STATIC)
target_include_directories(libuvncsettings PRIVATE "${CMAKE_SOURCE_DIR}/../DSMPlugin")
target_include_directories(libuvncsettings PRIVATE "${CMAKE_SOURCE_DIR}/../omnithread")
foreach(file ${LIBUVNCSETTINGS_SOURCES})
    target_sources(libuvncsettings PRIVATE "${CMAKE_SOURCE_DIR}/../uvnc_settings/uvnc_settings/${file}")
endforeach(file)
