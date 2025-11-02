cmake_minimum_required(VERSION 3.22.1)

project("librats_android")

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_library(log-lib log)
find_package(Threads REQUIRED)

# Try to pull in the root LibRats CMake to build the core library
set(LIBRATS_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../../)
set(LIBRATS_BINARY_DIR ${CMAKE_BINARY_DIR}/librats_native)

if(EXISTS "${LIBRATS_SOURCE_DIR}/CMakeLists.txt")
    # Configure LibRats options suitable for Android cross-compilation
    set(RATS_BUILD_EXAMPLES OFF CACHE BOOL "Disable examples for Android build" FORCE)
    set(RATS_BUILD_TESTS OFF CACHE BOOL "Disable tests for Android build" FORCE)
    set(RATS_CROSSCOMPILING ON CACHE BOOL "Indicate cross-compilation environment" FORCE)
    set(RATS_BINDINGS ON CACHE BOOL "Enable C bindings for JNI" FORCE)

    add_subdirectory(${LIBRATS_SOURCE_DIR} ${LIBRATS_BINARY_DIR})

    # Ensure Android-specific defines are present on the core target
    if(TARGET rats)
        target_compile_definitions(rats PRIVATE ANDROID __ANDROID__)
    else()
        message(FATAL_ERROR "LibRats target 'rats' was not created by root CMakeLists.txt")
    endif()

    # JNI wrapper library
    add_library(rats_jni SHARED librats_jni.cpp)

    # Include the LibRats public headers and generated version.h
    target_include_directories(rats_jni PRIVATE
        ${LIBRATS_SOURCE_DIR}/src
        ${LIBRATS_BINARY_DIR}/src
    )

    # Link JNI wrapper with librats and required libraries
    target_link_libraries(rats_jni
        rats
        ${log-lib}
    )

    # Optimization and common warnings for JNI wrapper
    target_compile_options(rats_jni PRIVATE
        -fvisibility=default
        -fPIC
        -O2
        -Wall
        -Wextra
        -Wno-unused-parameter
        -Wno-missing-field-initializers
    )

    # Export symbols for JNI
    set_target_properties(rats_jni PROPERTIES
        ANDROID_ARM_NEON TRUE
        LINK_FLAGS "-Wl,--export-dynamic"
    )
else()
    message(FATAL_ERROR "LibRats root CMakeLists.txt not found at ${LIBRATS_SOURCE_DIR}. Build the Android module from the repository root so native sources are available.")
endif()
