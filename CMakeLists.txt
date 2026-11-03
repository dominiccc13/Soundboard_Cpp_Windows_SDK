cmake_minimum_required(VERSION 3.25...3.50)
project(Soundboard CXX)

option(SB_STATIC_QT "Link a statically built Qt (needs a from-source static Qt; uses /MT)" OFF)

if(NOT CMAKE_PREFIX_PATH AND NOT DEFINED ENV{CMAKE_PREFIX_PATH})
    if(SB_STATIC_QT)
        set(CMAKE_PREFIX_PATH "C:/Qt/6.12.0/msvc2022_64_static")
    else()
        set(CMAKE_PREFIX_PATH "C:/Qt/6.12.0/msvc2022_64")
    endif()
endif()
find_package(Qt6 REQUIRED COMPONENTS Widgets)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    # CRT must match Qt: dynamic Qt is built /MD, static Qt is built /MT.
    if(SB_STATIC_QT)
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    else()
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
    endif()

    add_compile_options(/W4 /WX /permissive- /sdl /GS /guard:cf /Zc:__cplusplus)
    add_link_options(
        /guard:cf /DYNAMICBASE /HIGHENTROPYVA /NXCOMPAT /CETCOMPAT
        /INCREMENTAL:NO /OPT:REF /OPT:ICF
        # import-time DLL search: app dir + System32 only (no CWD / PATH)
        /DEPENDENTLOADFLAG:0xA00
    )
    # no debug info in shipped binaries
    set(CMAKE_CXX_FLAGS_RELEASE "/O2 /Ob2 /DNDEBUG")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "/DEBUG:NONE")
    set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "/DEBUG:NONE")
endif()

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
if(SB_STATIC_QT)
    target_link_libraries(SbWindow PRIVATE Qt6::QWindowsIntegrationPlugin Qt6::QModernWindowsStylePlugin)
endif()

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

add_library(ConvertAudio STATIC)
target_sources(ConvertAudio
    PRIVATE
        Main/ConvertAudio.cpp
    PUBLIC
        FILE_SET HEADERS
        FILES
            Main/ConvertAudio.h
)
target_link_libraries(ConvertAudio PRIVATE ole32 mfplat mfreadwrite mfuuid)

add_executable(soundboard
    Main/Main.cpp
    Resources/resource.rc
)

set_target_properties(soundboard PROPERTIES
    WIN32_EXECUTABLE TRUE
)

if(MSVC)
    # target_link_options(soundboard PRIVATE /ENTRY:mainCRTStartup)
    target_link_options(soundboard PRIVATE /SUBSYSTEM:CONSOLE)
endif()

target_link_libraries(soundboard
    PRIVATE
        Tray
        WasapiMixer
        SbWindow
        ConvertAudio
)

# ---- distributable: cmake --build build --config Release --target dist ----
# Static Qt: dist is just soundboard.exe. Dynamic Qt: exe + 3 Qt DLLs + 2 plugins + app-local VC runtime.
set(SB_DIST_DIR "${CMAKE_BINARY_DIR}/dist")
if(SB_STATIC_QT)
    add_custom_target(dist
        COMMAND ${CMAKE_COMMAND} -E make_directory "${SB_DIST_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:soundboard>" "${SB_DIST_DIR}"
        DEPENDS soundboard)
else()
    set(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS_SKIP TRUE)
    set(CMAKE_INSTALL_UCRT_LIBRARIES FALSE) # UCRT ships with Windows 10+
    include(InstallRequiredSystemLibraries)
    # Qt6/exe only import these; drop concrt140, msvcp140_atomic_wait, msvcp140_codecvt_ids
    list(FILTER CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS INCLUDE REGEX "(msvcp140(_1|_2)?|vcruntime140(_1)?)[.]dll$")
    set(SB_QT_PLUGINS "${Qt6Core_DIR}/../../../plugins")
    add_custom_target(dist
        COMMAND ${CMAKE_COMMAND} -E make_directory "${SB_DIST_DIR}/platforms" "${SB_DIST_DIR}/styles"
        COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:soundboard>" "$<TARGET_FILE:Qt6::Core>" "$<TARGET_FILE:Qt6::Gui>" "$<TARGET_FILE:Qt6::Widgets>" "${SB_DIST_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy "${SB_QT_PLUGINS}/platforms/qwindows.dll" "${SB_DIST_DIR}/platforms"
        COMMAND ${CMAKE_COMMAND} -E copy "${SB_QT_PLUGINS}/styles/qmodernwindowsstyle.dll" "${SB_DIST_DIR}/styles"
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS} "${SB_DIST_DIR}"
        DEPENDS soundboard)
endif()

# dev convenience: sample soundbites next to the exe (Main.cpp reads <exe dir>\Soundbites\)
add_custom_command(TARGET soundboard POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/Resources/TestSoundbites" "$<TARGET_FILE_DIR:soundboard>/Soundbites"
)
