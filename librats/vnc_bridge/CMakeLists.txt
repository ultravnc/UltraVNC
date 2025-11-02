# VNC Bridge - TCP to UDP tunnel using librats
cmake_minimum_required(VERSION 3.10)

# Create vnc_bridge executable
add_executable(vnc_bridge vnc_bridge.cpp)

# Link with librats library
target_link_libraries(vnc_bridge rats)

# Include directories (parent src folder has librats.h and logger.h)
target_include_directories(vnc_bridge PRIVATE
    ${PROJECT_SOURCE_DIR}/src
)

# Windows specific settings
if(WIN32)
    target_link_libraries(vnc_bridge ws2_32)
    
    # Add version information
    if(EXISTS "${PROJECT_SOURCE_DIR}/version.rc.in")
        set(VERSION_RC "${PROJECT_BINARY_DIR}/vnc_bridge_version.rc")
        configure_file(
            "${PROJECT_SOURCE_DIR}/version.rc.in"
            "${VERSION_RC}"
            @ONLY
        )
        target_sources(vnc_bridge PRIVATE "${VERSION_RC}")
    endif()
endif()

# Set output directory
set_target_properties(vnc_bridge PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin
)

# Install target
install(TARGETS vnc_bridge
    RUNTIME DESTINATION bin
)
