
project(setcad LANGUAGES CXX RC)

set(${PROJECT_NAME}_SOURCES
    setcad.cpp
    stdafx.cpp
)
add_executable(${PROJECT_NAME})
foreach(file ${${PROJECT_NAME}_SOURCES})
    target_sources(${PROJECT_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/../setcad/setcad/${file}")
endforeach(file)
target_sources(${PROJECT_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/../setcad/setcad/setcad.rc")
set_source_files_properties(${CMAKE_SOURCE_DIR}/../setcad/setcad/setcad.rc LANGUAGE RC)
target_compile_definitions(${PROJECT_NAME} PRIVATE WIN32 NDEBUG _CONSOLE UNICODE=1 _UNICODE=1)
if (MINGW)
    target_link_options(${PROJECT_NAME} PRIVATE "-municode")
endif()
install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${CMAKE_BINARY_DIR}/ultravnc)
if (MSVC)
    install(FILES $<TARGET_PDB_FILE:${PROJECT_NAME}> DESTINATION ${CMAKE_BINARY_DIR}/ultravnc OPTIONAL)
endif()
