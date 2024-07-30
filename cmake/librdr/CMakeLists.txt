find_package(ZLIB REQUIRED)
find_package(zstd CONFIG REQUIRED)

set(LIBRDR_SOURCES
    FdInStream.cxx
    ZlibInStream.cxx
    ZlibOutStream.cxx
    ZstdInStream.cxx
    ZstdOutStream.cxx
)
add_library(librdr STATIC)
target_compile_definitions(librdr PRIVATE _VCPKG)
foreach(file ${LIBRDR_SOURCES})
    target_sources(librdr PRIVATE "${CMAKE_SOURCE_DIR}/../rdr/${file}")
endforeach(file)
target_link_libraries(librdr
    PRIVATE ZLIB::ZLIB
    PRIVATE zstd::libzstd
)
