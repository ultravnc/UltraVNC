
project(createpassword LANGUAGES CXX RC)

set(${PROJECT_NAME}_SOURCES
    createpassword.cpp
    ../../setpasswd/setpasswd/inifile.cpp
)
add_executable(${PROJECT_NAME})
foreach(file ${${PROJECT_NAME}_SOURCES})
    target_sources(${PROJECT_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/../winvnc/createpassword/${file}")
endforeach(file)
install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${CMAKE_BINARY_DIR}/ultravnc)
if (MSVC)
    install(FILES $<TARGET_PDB_FILE:${PROJECT_NAME}> DESTINATION ${CMAKE_BINARY_DIR}/ultravnc OPTIONAL)
endif()
