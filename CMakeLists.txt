cmake_minimum_required(VERSION 3.25...3.50)
project(Soundboard CXX)
if(NOT CMAKE_PREFIX_PATH AND NOT DEFINED ENV{CMAKE_PREFIX_PATH})
    set(CMAKE_PREFIX_PATH "C:/Qt/6.12.0/msvc2022_64")
endif()
find_package(Qt6 REQUIRED COMPONENTS Widgets)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

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

add_library(SbWindow STATIC)
target_sources(SbWindow
    PRIVATE
        Main/SbWindow.cpp
    PUBLIC
        FILE_SET HEADERS
        FILES
            Main/SbWindow.h
)
target_link_libraries(SbWindow PRIVATE Qt6::Widgets Tray)
set_target_properties(SbWindow PROPERTIES CXX_STANDARD 20)

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
        SbWindow
)

add_custom_command(TARGET soundboard POST_BUILD
    COMMAND "${CMAKE_PREFIX_PATH}/bin/windeployqt.exe" --no-translations "$<TARGET_FILE:soundboard>"
)