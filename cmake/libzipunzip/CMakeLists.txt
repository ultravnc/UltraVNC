
set(LIBZIPUNZIP_SOURCES
    api.c
    crc32.c
    crctab.c
    crypt.c
    explode.c
    extract.c
    fileio.c
    globals.c
    inflate.c
    list.c
    match.c
    process.c
    unshrink.c
    win32/nt.c
    win32/win32.c
    windll/windll.c
    zipinfo.c
)
add_library(libzipunzip STATIC)
foreach(file ${LIBZIPUNZIP_SOURCES})
    target_sources(libzipunzip PRIVATE "${CMAKE_SOURCE_DIR}/../zipunzip_src/unzip/${file}")
endforeach(file)
target_compile_definitions(libzipunzip PRIVATE WINDLL DLL UNZIPLIB USE_EF_UT_TIME)
