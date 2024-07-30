
set(REPEATER_SOURCES
    gui.cpp
    lists_functions.cpp
    mode12_listener.cpp
    mode2_listener_server.cpp
    repeater.cpp
    socket_functions.cpp
    webgui/settings.c
    webgui/webclib.c
    webgui/webfs.c
    webgui/webio.c
    webgui/webobjs.c
    webgui/websys.c
    webgui/webtest.c
    webgui/webutils.c
    webgui/wsfdata.c
)
add_executable(repeater)
foreach(file ${REPEATER_SOURCES})
    target_sources(repeater PRIVATE "${CMAKE_SOURCE_DIR}/../repeater/${file}")
endforeach(file)
target_sources(repeater PRIVATE "${CMAKE_SOURCE_DIR}/../repeater/resources.rc")
set_source_files_properties(${CMAKE_SOURCE_DIR}/../repeater/resources.rc LANGUAGE RC)
set_source_files_properties(${CMAKE_SOURCE_DIR}/../repeater/resources.rc PROPERTIES COMPILE_FLAGS "-Ocoff")
target_link_libraries(repeater
    winmm
    ws2_32
)
