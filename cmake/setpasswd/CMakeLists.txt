
project(setpasswd LANGUAGES CXX RC)

set(${PROJECT_NAME}_SOURCES
    inifile.cpp
    setpasswd.cpp
    stdafx.cpp
)
add_executable(${PROJECT_NAME})
foreach(file ${${PROJECT_NAME}_SOURCES})
    target_sources(${PROJECT_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/../setpasswd/setpasswd/${file}")
endforeach(file)
target_sources(${PROJECT_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/../setpasswd/setpasswd/setpasswd.rc")
set_source_files_properties(${CMAKE_SOURCE_DIR}/../setpasswd/setpasswd/setpasswd.rc LANGUAGE RC)
target_compile_definitions(${PROJECT_NAME} PRIVATE WIN32 NDEBUG _CONSOLE)
install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${CMAKE_BINARY_DIR}/ultravnc)
if (MSVC)
    install(FILES $<TARGET_PDB_FILE:${PROJECT_NAME}> DESTINATION ${CMAKE_BINARY_DIR}/ultravnc OPTIONAL)
endif()
