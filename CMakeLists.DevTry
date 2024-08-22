[===[Adding Root CMakeLists for Qt Community Tests]===]
cmake_minimum_required(VERSION 3.16)

project(vncviewer VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)
qt_standard_project_setup()

qt_add_executable(vncviewer
	AboutBoxQt.ui
	AboutBoxQt.cpp
	mainwindow.ui
	mainwindow.cpp
	main.cpp
)

target_link_libraries(vncviewer PRIVATE Qt6::Widgets)

set_target_properties(vncviewer PROPERTIES
    WIN32_EXECUTABLE ON
    MACOSX_BUNDLE ON
)