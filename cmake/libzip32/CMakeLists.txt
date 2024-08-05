
project(libzip32 LANGUAGES CXX RC)

set(${PROJECT_NAME}_SOURCES
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
add_library(${PROJECT_NAME} STATIC)
foreach(file ${${PROJECT_NAME}_SOURCES})
    target_sources(${PROJECT_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/../zipunzip_src/zip20/${file}")
endforeach(file)
target_compile_definitions(${PROJECT_NAME} PRIVATE NDEBUG _WINDOWS WIN32 NO_ASM WINDLL MSDOS USE_ZIPMAIN ZIPLIB)
