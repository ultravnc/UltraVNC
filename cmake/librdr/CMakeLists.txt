
project(librdr LANGUAGES CXX RC)
# Patch Zlib Error for Visual Studio 2022
set(ZLIB_INCLUDE_DIR "C:/source/vcpkg/packages/zlib_x64-windows-static/include")
set(ZLIB_LIBRARY "C:/source/vcpkg/packages/zlib_x64-windows-static/lib/zlib.lib")
find_package(ZLIB REQUIRED)
# Patch zstd Error for Visual Studio 2022
set(ZSTD_INCLUDE_DIR "C:/source/vcpkg/packages/zstd_x64-windows-static/include")
set(ZSTD_LIBRARY "C:/source/vcpkg/packages/zstd_x64-windows-static/lib/zstd.lib")
include_directories(${ZSTD_INCLUDE_DIR})
link_directories("C:/source/vcpkg/packages/zstd_x64-windows-static/lib")
#find_package(zstd CONFIG REQUIRED) <-Disable CMake don't find
# Patch LibLZMA Error for Visual Studio 2022
set(LIBLZMA_INCLUDE_DIR "C:/source/vcpkg/packages/liblzma_x64-windows-static/include")
set(LIBLZMA_LIBRARY "C:/source/vcpkg/packages/liblzma_x64-windows-static/lib/lzma.lib")
find_package(LibLZMA REQUIRED)

set(${PROJECT_NAME}_SOURCES
    FdInStream.cxx
    FdOutStream.cxx
    InStream.cxx
    NullOutStream.cxx
    ZlibInStream.cxx
    ZlibOutStream.cxx
    ZstdInStream.cxx
    ZstdOutStream.cxx
    xzInStream.cxx
    xzOutStream.cxx
)
add_library(${PROJECT_NAME} STATIC)
target_compile_definitions(${PROJECT_NAME} PRIVATE _XZ _VCPKG)
foreach(file ${${PROJECT_NAME}_SOURCES})
    target_sources(${PROJECT_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/../rdr/${file}")
endforeach(file)
target_link_libraries(${PROJECT_NAME}
    PRIVATE ZLIB::ZLIB
    #PRIVATE zstd::libzstd              # Patch zstd Error for Visual Studio 2022
    PRIVATE ${ZSTD_LIBRARY}::libzstd    # Patch zstd Error for Visual Studio 2022
    PRIVATE LibLZMA::LibLZMA
)
