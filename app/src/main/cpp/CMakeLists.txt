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
    geometry/mesh.cpp
    geometry/material.cpp

    # World System (Phase 3+)
    world/world_manager.cpp
    world/cell.cpp
    world/cell_loader.cpp
    world/cell_transition_manager.cpp
    world/grid.cpp
    world/terrain.cpp

    # Game Systems
    game/npc.cpp
    game/npc_manager.cpp
    game/combat_manager.cpp
    game/quest_manager.cpp
    game/spell_manager.cpp

    # Player & Movement (Phase 3+)
    game/player.cpp
    game/player_controller.cpp
    input/touch_input_handler.cpp

    # Inventory System (Phase 3+)
    game/inventory.cpp
    game/inventory_manager.cpp

    # UI Systems
    ui/title_screen.cpp
    ui/quest_ui.cpp
    ui/inventory_ui.cpp
    ui/text_renderer.cpp
    ui/debug_hud.cpp
    ui/settings_ui.cpp

    # System
    system/settings_manager.cpp

    # Localization
    localization/localization_manager.cpp

    # Profiling
    profiling/performance_monitor.cpp

    # Save System
    save_system/save_manager.cpp

    # Asset System
    assets/nif_parser.cpp
    assets/dds_loader.cpp
    assets/asset_manager.cpp

    # JNI Bridge (Java/C++ communication)
    jni_bridge.cpp
)

# Include directories
target_include_directories(native-lib PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/engine
    ${CMAKE_CURRENT_SOURCE_DIR}/game
    ${CMAKE_CURRENT_SOURCE_DIR}/world
    ${CMAKE_CURRENT_SOURCE_DIR}/player
    ${CMAKE_CURRENT_SOURCE_DIR}/input
    ${CMAKE_CURRENT_SOURCE_DIR}/ui
    ${CMAKE_CURRENT_SOURCE_DIR}/geometry
    ${CMAKE_CURRENT_SOURCE_DIR}/math
    ${CMAKE_CURRENT_SOURCE_DIR}/localization
    ${CMAKE_CURRENT_SOURCE_DIR}/profiling
    ${CMAKE_CURRENT_SOURCE_DIR}/save_system
    ${CMAKE_CURRENT_SOURCE_DIR}/audio
    ${CMAKE_CURRENT_SOURCE_DIR}/assets
)

# Find OpenAL-Soft (optional for now)
find_package(OpenAL CONFIG QUIET)
if(NOT OpenAL_FOUND)
    # Try to find OpenAL in standard paths
    find_package(OpenAL QUIET)
endif()

# Audio system: Using Android MediaPlayer via JNI bridge (no OpenAL dependency)
# Message output for build info
if(OpenAL_FOUND)
    message(STATUS "OpenAL found but not used - JNI bridge approach")
else()
    message(STATUS "Using Android MediaPlayer via JNI for audio playback")
endif()

# Link Android native libraries
target_link_libraries(native-lib
    android
    EGL
    GLESv3
    log
)

# Link OpenAL if found
if(OpenAL_FOUND)
    target_link_libraries(native-lib OpenAL::OpenAL)
endif()

# Compiler flags
if (ANDROID_ABI STREQUAL arm64-v8a)
    target_compile_options(native-lib PRIVATE -O3 -march=armv8-a -Wno-macro-redefined)
elseif (ANDROID_ABI STREQUAL armeabi-v7a)
    target_compile_options(native-lib PRIVATE -O3 -march=armv7-a -mfloat-abi=softfp -Wno-macro-redefined)
elseif (ANDROID_ABI STREQUAL x86)
    target_compile_options(native-lib PRIVATE -O3 -march=i686 -Wno-macro-redefined)
elseif (ANDROID_ABI STREQUAL x86_64)
    target_compile_options(native-lib PRIVATE -O3 -march=x86-64 -Wno-macro-redefined)
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
