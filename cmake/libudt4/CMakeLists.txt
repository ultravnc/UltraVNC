
set(LIBUDT4_SOURCES
    src/api.cpp
    src/buffer.cpp
    src/cache.cpp
    src/ccc.cpp
    src/channel.cpp
    src/common.cpp
    src/core.cpp
    src/epoll.cpp
    src/list.cpp
    src/md5.cpp
    src/packet.cpp
    src/queue.cpp
    src/window.cpp
)
add_library(libudt4 STATIC)
target_include_directories(libudt4 PRIVATE "${CMAKE_SOURCE_DIR}/../udt4")
foreach(file ${LIBUDT4_SOURCES})
    target_sources(libudt4 PRIVATE "${CMAKE_SOURCE_DIR}/../udt4/${file}")
endforeach(file)
target_link_libraries(libudt4
    ws2_32
    wsock32
)
