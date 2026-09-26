cmake_minimum_required(VERSION 3.25...3.50)
project(Soundboard CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Qt6 6.11 REQUIRED COMPONENTS Core Widgets)

add_library(Tray STATIC)
target_sources(Tray
    PRIVATE
        Main/Tray.cpp
    PUBLIC
        FILE_SET HEADERS
        FILES
            Main/Tray.h
)
target_link_libraries(Tray PRIVATE user32)
target_link_libraries(Tray PRIVATE shell32)

add_library(WasapiMixer STATIC)
target_sources(WasapiMixer
    PRIVATE
        Main/WasapiMixer.cpp
    PUBLIC
        FILE_SET HEADERS
        FILES
            Main/WasapiMixer.h
)
target_link_libraries(WasapiMixer PRIVATE ole32)

add_library(Utils STATIC)
target_sources(Utils
    PRIVATE
        Main/Utils.cpp
    PUBLIC
        FILE_SET HEADERS
        FILES
            Main/Utils.h
)

add_library(GUI STATIC)
target_sources(GUI
    PRIVATE
        Main/GUI.cpp
    PUBLIC
        FILE_SET HEADERS
        FILES
            Main/GUI.h
)
target_link_libraries(GUI PRIVATE Qt6::Core Qt6::Widgets)
set_target_properties(GUI PROPERTIES AUTOMOC ON)

add_executable(soundboard
    Main/Main.cpp
    Resources/resource.rc
)

set_target_properties(soundboard PROPERTIES
    WIN32_EXECUTABLE TRUE
)

if(MSVC)
    target_link_options(soundboard PRIVATE /ENTRY:mainCRTStartup)
endif()

target_link_libraries(soundboard
    PRIVATE
        Tray
        WasapiMixer
        Utils
        GUI
        Qt6::Core
        Qt6::Widgets
)