cmake_minimum_required(VERSION 3.1.0)

project(vnchooks LANGUAGES C CXX RC)

set(CMAKE_C_FLAGS "-g -O0 -Wno-write-strings")
set(CMAKE_CXX_FLAGS "-g -O0 -Wno-write-strings -Wno-conversion-null")



set(VNCVIEWER_SOURCES
    VNCHooks.cpp
)
add_library(vnchooks SHARED)
set_target_properties(vnchooks PROPERTIES PREFIX "")
foreach(file ${VNCVIEWER_SOURCES})
    target_sources(vnchooks PRIVATE "${CMAKE_SOURCE_DIR}/../winvnc/vnchooks/${file}")
endforeach(file)
target_sources(vnchooks PRIVATE "${CMAKE_SOURCE_DIR}/../winvnc/vnchooks/vnchooks.rc")
set_source_files_properties(${CMAKE_SOURCE_DIR}/../winvnc/vnchooks/vnchooks.rc LANGUAGE RC)
set_source_files_properties(${CMAKE_SOURCE_DIR}/../winvnc/vnchooks/vnchooks.rc PROPERTIES COMPILE_FLAGS "-Ocoff")
