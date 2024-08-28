
project(vncviewer LANGUAGES C CXX RC)


# Original vncviewer Source
#if source file compile with msvc (or legacy compiler of vncviewer)
# LegacyVncViewer
if (MINGW or "${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
	find_package(JPEG REQUIRED)
	find_package(libjpeg-turbo CONFIG REQUIRED)
	find_package(ZLIB REQUIRED)
	find_package(zstd CONFIG REQUIRED)
	find_package(LibLZMA REQUIRED)
	
	set(${PROJECT_NAME}_SOURCES
		../DSMPlugin/DSMPlugin.cpp
		../ZipUnZip32/ZipUnzip32.cpp
		../common/Clipboard.cpp
		../common/Hyperlinks.cpp
		../common/UltraVncZ.cpp
		../common/mnemonic.c
		../common/mn_wordlist.c
		../common/d3des.c
		../common/win32_helpers.cpp
		../lzo/minilzo.c
		../rfb/dh.cpp
		AboutBox.cpp
		AccelKeys.cpp
		AuthDialog.cpp
		ClientConnection.cpp
		ClientConnectionCacheRect.cpp
		ClientConnectionClipboard.cpp
		ClientConnectionCoRRE.cpp
		ClientConnectionCopyRect.cpp
		ClientConnectionCursor.cpp
		ClientConnectionFile.cpp
		ClientConnectionFullScreen.cpp
		ClientConnectionHextile.cpp
		ClientConnectionRRE.cpp
		ClientConnectionRSAAES.cpp
		ClientConnectionRaw.cpp
		ClientConnectionTLS.cpp
		ClientConnectionTight.cpp
		ClientConnectionUltra.cpp
		ClientConnectionUltra2.cpp
		ClientConnectionZlib.cpp
		ClientConnectionZlibHex.cpp
		Daemon.cpp
		Exception.cpp
		FileTransfer.cpp
		FullScreenTitleBar.cpp
		KeyMap.cpp
		KeyMapjap.cpp
		Log.cpp
		LowLevelHook.cpp
		MRU.cpp
		InfoBox.cpp
		SessionDialog.cpp
		SessionDialogTabs.cpp
		Snapshot.cpp
		TextChat.cpp
		UltraVNCHerperFunctions.cpp
		VNCOptions.cpp
		VNCviewerApp.cpp
		VNCviewerApp32.cpp
		buildtime.cpp
		directx/directxviewer.cpp
		display.cpp
		sessiondialogLoadSave.cpp
		stdhdrs.cpp
		vncauth.c
		vnctouch.cpp
		vncviewer.cpp
		xz.cpp
		zrle.cpp
	)
	add_executable(${PROJECT_NAME} WIN32)
	target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/..")
	target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/../omnithread")
	foreach(file ${${PROJECT_NAME}_SOURCES})
		target_sources(${PROJECT_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/../vncviewer/${file}")
	endforeach(file)
	target_sources(${PROJECT_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/../vncviewer/res/vncviewer.rc")
	set_source_files_properties(${CMAKE_SOURCE_DIR}/../vncviewer/res/vncviewer.rc LANGUAGE RC)
	target_compile_definitions(${PROJECT_NAME} PRIVATE NDEBUG WIN32 _WINDOWS __NT__ _WINSTATIC __WIN32__ _VIEWER _XZ _CLOUD _VCPKG NOMINMAX)
	target_link_libraries(${PROJECT_NAME}
		PRIVATE JPEG::JPEG
		PRIVATE $<IF:$<TARGET_EXISTS:libjpeg-turbo::turbojpeg>,libjpeg-turbo::turbojpeg,libjpeg-turbo::turbojpeg-static>
		PRIVATE ZLIB::ZLIB
		PRIVATE zstd::libzstd
		PRIVATE LibLZMA::LibLZMA
		libomnithread
		librdr
		libudtcloud
		libzip32
		libzipunzip
		comctl32
		comdlg32
		crypt32
		cryptui
		gdi32
		gdiplus
		imm32
		secur32
		shlwapi
		version
		winmm
		ws2_32
	)
	install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${CMAKE_BINARY_DIR}/ultravnc)
	if (MSVC)
		install(FILES $<TARGET_PDB_FILE:${PROJECT_NAME}> DESTINATION ${CMAKE_BINARY_DIR}/ultravnc OPTIONAL)
	endif()
# End LegacyVncViewer

# Qt Community Tests VncViewer Source (In Progress)
# elseif source file compile with Qt
# CuteVncViewer
else
	#
	# Inclusion of Project Files
	# 
	#set(FORMS_DIR "${CMAKE_SOURCE_DIR}/forms")
	set(FORMS_DIR "${CMAKE_SOURCE_DIR}")
	#set(INCLUDE_DIR "${CMAKE_SOURCE_DIR}/include")
	set(INCLUDE_DIR "${CMAKE_SOURCE_DIR}")
	#set(SOURCES_DIR "${CMAKE_SOURCE_DIR}/src")
	set(SOURCES_DIR "${CMAKE_SOURCE_DIR}")
	# Create the folders if doesn't exist (nothing to create all are on the same path)
	
	include_directories(${FORMS_DIR})
	include_directories(${INCLUDE_DIR})
	include_directories(${SOURCE_DIR})
	
	file(GLOB_RECURSE SOURCES
		"${FORMS_DIR}/*.ui"
		"${FORMS_DIR}/*.qrc"
		"${INCLUDE_DIR}/*.h"
		"${SOURCE_DIR}/*.cpp"
	)
	
# End CuteVncViewer
endif()