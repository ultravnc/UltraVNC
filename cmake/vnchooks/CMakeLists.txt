
project(vnchooks LANGUAGES CXX RC)

set(${PROJECT_NAME}_SOURCES
    VNCHooks.cpp
)
add_library(${PROJECT_NAME} SHARED)
foreach(file ${${PROJECT_NAME}_SOURCES})
    target_sources(${PROJECT_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/../winvnc/vnchooks/${file}")
endforeach(file)
target_sources(${PROJECT_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/../winvnc/vnchooks/vnchooks.rc")
set_source_files_properties(${CMAKE_SOURCE_DIR}/../winvnc/vnchooks/vnchooks.rc LANGUAGE RC)
target_compile_definitions(${PROJECT_NAME} PRIVATE NDEBUG __WIN32__ WIN32 _WINDOWS _USRDLL VNCHOOKS_EXPORTS _WIN32_WINNT=0x400)
set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")
install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${CMAKE_BINARY_DIR}/ultravnc)
if (MSVC)
    install(FILES $<TARGET_PDB_FILE:${PROJECT_NAME}> DESTINATION ${CMAKE_BINARY_DIR}/ultravnc OPTIONAL)
endif()
