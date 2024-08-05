
project(authadmin LANGUAGES CXX RC)

set(${PROJECT_NAME}_SOURCES
    authadmin.cpp
)
add_library(${PROJECT_NAME} SHARED)
foreach(file ${${PROJECT_NAME}_SOURCES})
    target_sources(${PROJECT_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/../addon/ms-logon/authadm/${file}")
endforeach(file)
target_sources(${PROJECT_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/../addon/ms-logon/authadm/authadmin.rc")
set_source_files_properties(${CMAKE_SOURCE_DIR}/../addon/ms-logon/authadm/authadmin.rc LANGUAGE RC)
target_compile_definitions(${PROJECT_NAME} PRIVATE WIN32 NDEBUG _WINDOWS AUTHADMIN_EXPORTS)
set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")
target_link_libraries(${PROJECT_NAME}
    netapi32
)
install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${CMAKE_BINARY_DIR}/ultravnc)
if (MSVC)
    install(FILES $<TARGET_PDB_FILE:${PROJECT_NAME}> DESTINATION ${CMAKE_BINARY_DIR}/ultravnc OPTIONAL)
endif()
