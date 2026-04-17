cmake_minimum_required(VERSION 3.18.1)
project(oblivion_native)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add main library
add_library(native-lib SHARED
    # Engine Core
    engine/renderer.cpp
    engine/shader.cpp
    engine/camera.cpp

    # Geometry
    geometry/cube.cpp

    # Game Systems
    game/npc.cpp
    game/npc_manager.cpp
    game/world_manager.cpp
    game/combat_manager.cpp
    game/quest_manager.cpp
    game/spell_manager.cpp

    # UI Systems
    ui/title_screen.cpp
    ui/quest_ui.cpp

    # Localization
    localization/localization_manager.cpp

    # Profiling
    profiling/performance_monitor.cpp

    # Save System
    save_system/save_manager.cpp

    # JNI Bridge
    jni_bridge.cpp
)

# Include directories
target_include_directories(native-lib PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/engine
    ${CMAKE_CURRENT_SOURCE_DIR}/game
    ${CMAKE_CURRENT_SOURCE_DIR}/ui
    ${CMAKE_CURRENT_SOURCE_DIR}/geometry
    ${CMAKE_CURRENT_SOURCE_DIR}/math
    ${CMAKE_CURRENT_SOURCE_DIR}/localization
    ${CMAKE_CURRENT_SOURCE_DIR}/profiling
    ${CMAKE_CURRENT_SOURCE_DIR}/save_system
)

# Link Android native libraries
target_link_libraries(native-lib
    android
    EGL
    GLESv3
    log
)

# Compiler flags
if (ANDROID_ABI STREQUAL arm64-v8a)
    target_compile_options(native-lib PRIVATE -O3 -march=armv8-a)
elseif (ANDROID_ABI STREQUAL armeabi-v7a)
    target_compile_options(native-lib PRIVATE -O3 -march=armv7-a -mfloat-abi=softfp)
elseif (ANDROID_ABI STREQUAL x86)
    target_compile_options(native-lib PRIVATE -O3 -march=i686)
elseif (ANDROID_ABI STREQUAL x86_64)
    target_compile_options(native-lib PRIVATE -O3 -march=x86-64)
endif ()

# Common flags
target_compile_options(native-lib PRIVATE
    -Wall
    -Wextra
    -Werror=format
)

# Warning suppression for specific headers
set_source_files_properties(
    game/npc_manager.cpp
    game/quest_manager.cpp
    PROPERTIES
    COMPILE_FLAGS "-Wno-unused-variable"
)
