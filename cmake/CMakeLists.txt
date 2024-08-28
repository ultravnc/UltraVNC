cmake_minimum_required(VERSION 3.5.0)

project(UltraVNC LANGUAGES C CXX RC)


# Original UltraVNC Source
#if source file compile with msvc (or legacy compiler of UltraVNC)
# LegacyUltraVNC
if (MINGW or "${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
	set(CMAKE_CXX_STANDARD 14)
	if (MINGW)
		set(CMAKE_C_FLAGS "-Wno-write-strings")
		set(CMAKE_CXX_FLAGS "-Wno-write-strings -Wno-conversion-null")
	endif()
	if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
		add_compile_definitions("_CRT_SECURE_NO_WARNINGS")
		add_compile_definitions("_WINSOCK_DEPRECATED_NO_WARNINGS")
		add_compile_definitions("NOMINMAX")
		set(CMAKE_MFC_FLAG 2)
	endif()
	if( ${CMAKE_SYSTEM_PROCESSOR} STREQUAL "x86_64" OR ${CMAKE_SYSTEM_PROCESSOR} STREQUAL "AMD64" )
		add_compile_definitions("_X64")
	endif()
	
	option(asan "Build with address sanitizer enabled.")
	if (asan)
		add_compile_options(-fsanitize=address)
		add_link_options(-fsanitize=address)
		install(FILES "c:/Program Files/Microsoft Visual Studio/2022/Community/VC/Tools/MSVC/14.40.33807/bin/Hostx64/x64/clang_rt.asan_dynamic-x86_64.dll" DESTINATION ${CMAKE_BINARY_DIR}/ultravnc OPTIONAL)
	endif()
	
	if (MINGW)
		install(FILES /usr/lib/gcc/x86_64-w64-mingw32/13-win32/libstdc++-6.dll DESTINATION ${CMAKE_BINARY_DIR}/ultravnc OPTIONAL)
		install(FILES /usr/lib/gcc/x86_64-w64-mingw32/13-win32/libgcc_s_seh-1.dll DESTINATION ${CMAKE_BINARY_DIR}/ultravnc OPTIONAL)
	endif()
# End LegacyUltraVNC
 
# Qt Community Tests UltraVNC Source (In Progress)
# elseif source file compile with Qt
# CuteUltraVNC
else
	set(CMAKE_CXX_STANDARD 17)
	
	if (WIN32)
		project(ultravnc LANGUAGES CXX)
	elseif (UNIX)
		project(ultravnc)
	endif()

	set(CMAKE_CONFIGURATION_TYPES "Release;ReWithDebInfo" CACHE STRING "" FORCE)
	
	#
	# Inclusion of Qt
	#
	set(CMAKE_INCLUDE_CURRENT_DIR ON)
	set(CMAKE_AUTOMOC ON)
	set(CMAKE_AUTOUIC ON)
	set(CMAKE_PREFIX_PATH $ENV{QTDIR})
	find_package(Qt6Core REQUIRED)
	find_package(Qt6Widgets REQUIRED)
	# And all packages Qt6 you need
	
	# Not use on that level of CMake Qt
	##
	## Inclusion of Project Files
	## 
	#set(FORMS_DIR "${CMAKE_SOURCE_DIR}/forms")
	#set(INCLUDE_DIR "${CMAKE_SOURCE_DIR}/include")
	#set(SOURCES_DIR "${CMAKE_SOURCE_DIR}/src")
	## Create the folders if doesn't exist
	#
	#include_directories(${FORMS_DIR})
	#include_directories(${INCLUDE_DIR})
	#include_directories(${SOURCE_DIR})
	#
	#file(GLOB_RECURSE SOURCES
	#	"${FORMS_DIR}/*.ui"
	#	"${FORMS_DIR}/*.qrc"
	#	"${INCLUDE_DIR}/*.h"
	#	"${SOURCE_DIR}/*.cpp"
	#)
	
	#
	# Setup exectuable
	#
	# Enable debug logging on RELWITHDEBINFO configuration
	set_property(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS
		$<$<CONFIG:RELWITHDEBINFO>:QT_MESSAGELOGCONTEXT>
	)
	
	# Add the forms directory to the AUTOUIC search
	set(CMAKE_AUTOUIC_SEARCH_PATHS ${CMAKE_AUTOUIC_SEARCH_PATHS} ${FORMS_DIR}
	
	# Add the executable
	if (WIN32)
		add_executable(ultravnc WIN32 ${SOURCES})
	elseif (UNIX)
		add_executable(ultravnc ${SOURCES})
	endif()
	
	# Add the target includes for ultravnc
	target_include_directories(ultravnc PRIVATE ${FORMS_DIR})
	target_include_directories(ultravnc PRIVATE ${INCLUDE_DIR})
	target_include_directories(ultravnc PRIVATE ${SOURCE_DIR})
	
	# Linking Libraries
	target_link_libraries(ultravnc Qt6::Widgets)
	
# End CuteUltraVNC
endif()

# Original UltraVNC Source
add_subdirectory(authadmin)
add_subdirectory(authSSP)
add_subdirectory(createpassword)
add_subdirectory(ldapauth)
add_subdirectory(ldapauth9x)
add_subdirectory(ldapauthnt4)
add_subdirectory(libomnithread)
add_subdirectory(librdr)
add_subdirectory(libudt4)
add_subdirectory(libudtcloud)
add_subdirectory(libzip32)
add_subdirectory(libzipunzip)
add_subdirectory(logging)
add_subdirectory(logmessages)
add_subdirectory(MSLogonACL)
add_subdirectory(repeater)
#add_subdirectory(SecureVNCPlugin) # Not publicly available.
add_subdirectory(setcad)
add_subdirectory(setpasswd)
add_subdirectory(testauth)
add_subdirectory(uvnc_settings)
add_subdirectory(uvnckeyboardhelper)
add_subdirectory(vnchooks)
add_subdirectory(vncviewer)
add_subdirectory(winvnc)
add_subdirectory(workgrpdomnt4)
