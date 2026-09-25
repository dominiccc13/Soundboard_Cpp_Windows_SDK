cmake_minimum_required(VERSION 3.25...3.50)
project(Soundboard CXX)

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

add_executable(soundboard
    Main/Main.cpp
)

target_link_libraries(soundboard
    PRIVATE
        Tray
        WasapiMixer
        Utils
)