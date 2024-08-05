
project(librdr LANGUAGES CXX RC)

find_package(ZLIB REQUIRED)
find_package(zstd CONFIG REQUIRED)
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
    PRIVATE zstd::libzstd
    PRIVATE LibLZMA::LibLZMA
)
