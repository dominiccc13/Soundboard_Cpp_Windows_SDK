cmake_minimum_required(VERSION 3.25...3.50)
project(Soundboard CXX)

add_library(Tray)
target_sources(Tray
    PRIVATE
        Main/Tray.cpp
    PUBLIC
        FILE_SET HEADERS
        FILES
            Main/Tray.h
)

add_library(WasapiMixer)
target_sources(WasapiMixer
    PRIVATE
        Main/WasapiMixer.cpp
    PUBLIC
        FILE_SET HEADERS
        FILES
            Main/WasapiMixer.h
)

add_library(Utils)
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