
set(LIBZIP32_SOURCES
    api.c
    crypt.c
    deflate.c
    fileio.c
    globals.c
    trees.c
    util.c
    win32/nt.c
    win32/win32.c
    win32/win32zip.c
    windll/windll.c
    zip.c
    zipfile.c
    zipup.c
)
add_library(libzip32 STATIC)
foreach(file ${LIBZIP32_SOURCES})
    target_sources(libzip32 PRIVATE "${CMAKE_SOURCE_DIR}/../zipunzip_src/zip20/${file}")
endforeach(file)
target_compile_definitions(libzip32 PRIVATE NO_ASM WINDLL MSDOS USE_ZIPMAIN ZIPLIB USE_MINGW_GLOBBING)
