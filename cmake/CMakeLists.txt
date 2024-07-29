cmake_minimum_required(VERSION 3.5.0)

project(UltraVNC LANGUAGES C CXX RC)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_C_FLAGS "-Wno-write-strings")
set(CMAKE_CXX_FLAGS "-Wno-write-strings -Wno-conversion-null")

add_subdirectory(libomnithread)
add_subdirectory(librdr)
add_subdirectory(libudt4)
add_subdirectory(libudtcloud)
add_subdirectory(libuvncsettings)
add_subdirectory(libzip32)
add_subdirectory(libzipunzip)
add_subdirectory(uvnc_settings)
add_subdirectory(vncviewer)
add_subdirectory(winvnc)
