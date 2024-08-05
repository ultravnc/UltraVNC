
project(uvnc_settings LANGUAGES CXX RC)

set(${PROJECT_NAME}_SOURCES
    capture.cpp
    connections.cpp
    createsfx2.cpp
    createsfx3.cpp
    dsmplugin.cpp
    filetransfer.cpp
    firewall.cpp
    inifile.cpp
    log.cpp
    misc.cpp
    network.cpp
    security.cpp
    service.cpp
    stdafx.cpp
    upnp.cpp
    uvnc_settings.cpp
    videodrivercheck.cpp
    vncOSVersion.cpp
    vncsetauth.cpp
)
add_executable(${PROJECT_NAME} WIN32)
target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/../DSMPlugin")
target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/../omnithread")
foreach(file ${${PROJECT_NAME}_SOURCES})
    target_sources(${PROJECT_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/../uvnc_settings/uvnc_settings/${file}")
endforeach(file)
target_sources(${PROJECT_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/../uvnc_settings/uvnc_settings/uvnc_settings.rc")
set_source_files_properties(${CMAKE_SOURCE_DIR}/../uvnc_settings/uvnc_settings/uvnc_settings.rc LANGUAGE RC)
target_link_libraries(${PROJECT_NAME}
    libomnithread
    comctl32
    version
)
install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${CMAKE_BINARY_DIR}/ultravnc)
if (MSVC)
    install(FILES $<TARGET_PDB_FILE:${PROJECT_NAME}> DESTINATION ${CMAKE_BINARY_DIR}/ultravnc OPTIONAL)
endif()
