
project(MSLogonACL LANGUAGES CXX RC)

set(${PROJECT_NAME}_SOURCES
    MSLogonACL.cpp
    vncExportACL.cpp
    vncImportACL.cpp
)
add_executable(${PROJECT_NAME})
foreach(file ${${PROJECT_NAME}_SOURCES})
    target_sources(${PROJECT_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/../addon/ms-logon/MSLogonACL/${file}")
endforeach(file)
target_sources(${PROJECT_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/../addon/ms-logon/MSLogonACL/MSLogonACL.rc")
set_source_files_properties(${CMAKE_SOURCE_DIR}/../addon/ms-logon/MSLogonACL/MSLogonACL.rc LANGUAGE RC)
target_compile_definitions(${PROJECT_NAME} PRIVATE UNICODE=1 _UNICODE=1)
if (MINGW)
    target_link_options(${PROJECT_NAME} PRIVATE "-municode")
endif()
target_link_libraries(${PROJECT_NAME}
    netapi32
)
install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${CMAKE_BINARY_DIR}/ultravnc)
if (MSVC)
    install(FILES $<TARGET_PDB_FILE:${PROJECT_NAME}> DESTINATION ${CMAKE_BINARY_DIR}/ultravnc OPTIONAL)
endif()
