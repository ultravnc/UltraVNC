# Disable the 2025-02-09 18h02 UMT+1
#project(libudtcloud LANGUAGES CXX RC)
#
#set(${PROJECT_NAME}_SOURCES
#    proxy/Cloudthread.cpp
#    proxy/proxy.cpp
#    proxy/ringbuffer.cpp
#)
#add_library(${PROJECT_NAME} STATIC)
#target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/../UdtCloudlib")
#target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/../UdtCloudlib/proxy")
#foreach(file ${${PROJECT_NAME}_SOURCES})
#    target_sources(${PROJECT_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/../UdtCloudlib/${file}")
#endforeach(file)
#target_link_libraries(${PROJECT_NAME}
#    libudt4
#)
#