
project(ultravnc_service LANGUAGES CXX RC)

set(${PROJECT_NAME}_SOURCES
    src/UltraVNCService.cpp
    src/UltraVNCServiceClass.cpp
)
add_library(${PROJECT_NAME} STATIC)
target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/../ultravnc_service")
foreach(file ${${PROJECT_NAME}_SOURCES})
    target_sources(${PROJECT_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/../ultravnc_service/${file}")
endforeach(file)
target_link_libraries(${PROJECT_NAME}
	comctl32
	userenv
	wtsapi32
)