
set(LIBUDTCLOUD_SOURCES
    proxy/Cloudthread.cpp
    proxy/proxy.cpp
    proxy/ringbuffer.cpp
)
add_library(libudtcloud STATIC)
target_include_directories(libudtcloud PRIVATE "${CMAKE_SOURCE_DIR}/../UdtCloudlib")
target_include_directories(libudtcloud PRIVATE "${CMAKE_SOURCE_DIR}/../UdtCloudlib/proxy")
foreach(file ${LIBUDTCLOUD_SOURCES})
    target_sources(libudtcloud PRIVATE "${CMAKE_SOURCE_DIR}/../UdtCloudlib/${file}")
endforeach(file)
target_link_libraries(libudtcloud
    libudt4
)
