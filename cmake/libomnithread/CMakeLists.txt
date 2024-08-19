
project(libomnithread LANGUAGES CXX RC)

set(${PROJECT_NAME}_SOURCES
    nt.cpp
)
add_library(${PROJECT_NAME} STATIC)
foreach(file ${${PROJECT_NAME}_SOURCES})
    target_sources(${PROJECT_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/../omnithread/${file}")
endforeach(file)
target_compile_definitions(${PROJECT_NAME} PRIVATE NDEBUG WIN32 _WINDOWS __NT__ _WINSTATIC __WIN32__ _WIN32_WINNT=0x400)
