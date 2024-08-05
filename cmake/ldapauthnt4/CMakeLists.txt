
project(ldapauthnt4 LANGUAGES CXX RC)

set(${PROJECT_NAME}_SOURCES
    ldapAuthnt4.cpp
)
add_library(${PROJECT_NAME} SHARED)
foreach(file ${${PROJECT_NAME}_SOURCES})
    target_sources(${PROJECT_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/../addon/ms-logon/ldapauthNT4/${file}")
endforeach(file)
target_sources(${PROJECT_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/../addon/ms-logon/ldapauthNT4/ldapAuthnt4.rc")
set_source_files_properties(${CMAKE_SOURCE_DIR}/../addon/ms-logon/ldapauthNT4/ldapAuthnt4.rc LANGUAGE RC)
target_compile_definitions(${PROJECT_NAME} PRIVATE NDEBUG WIN32 _WINDOWS LDAPAUTHNT4_EXPORTS UNICODE=1 _UNICODE=1)
if (MINGW)
    target_link_options(${PROJECT_NAME} PRIVATE "-municode")
endif()
set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")
target_link_libraries(${PROJECT_NAME}
    activeds
    adsiid
    netapi32
)
install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${CMAKE_BINARY_DIR}/ultravnc)
if (MSVC)
    install(FILES $<TARGET_PDB_FILE:${PROJECT_NAME}> DESTINATION ${CMAKE_BINARY_DIR}/ultravnc OPTIONAL)
endif()
