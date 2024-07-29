
set(LIBOMNITHREAD_SOURCES
    nt.cpp
)
add_library(libomnithread STATIC)
foreach(file ${LIBOMNITHREAD_SOURCES})
    target_sources(libomnithread PRIVATE "${CMAKE_SOURCE_DIR}/../omnithread/${file}")
endforeach(file)
