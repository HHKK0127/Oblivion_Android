cmake_minimum_required(VERSION 3.18.1)
project(oblivion_native)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add nlohmann/json (header-only library)
# Check if already downloaded to avoid slow network fetch on every build
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/nlohmann/json.hpp")
    message(STATUS "nlohmann/json found locally")
else()
    include(FetchContent)
    FetchContent_Declare(json
        URL https://github.com/nlohmann/json/releases/download/v3.11.2/json.hpp
        DOWNLOAD_NO_EXTRACT TRUE
        DOWNLOAD_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/nlohmann
    )
    FetchContent_MakeAvailable(json)
endif()

# Add GLM (header-only math library)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/glm/glm.hpp")
    message(STATUS "GLM found locally")
else()
    message(FATAL_ERROR "GLM not found. Please download glm headers to external/glm/")
endif()

# Add main library
add_library(native-lib SHARED
    # Engine Core
    engine/renderer.cpp
    engine/shader.cpp
    engine/camera.cpp
    engine/graphics/retro_filter.cpp

    # Geometry
    geometry/cube.cpp
    geometry/mesh.cpp
    geometry/material.cpp

    # World System (Phase 3+)
    world/world_manager.cpp
    world/cell.cpp
    world/cell_loader.cpp
    world/grid.cpp
    world/terrain.cpp

    # Game Systems
    game/npc.cpp
    game/npc_manager.cpp
    game/combat_manager.cpp
    game/quest_manager.cpp
    game/spell_manager.cpp
    game/game_manager.cpp
    game/ai_state_machine.cpp
    game/dialogue_system.cpp
    game/interaction_manager.cpp
    game/test_scenario.cpp
    game/book_database.cpp

    # UI Systems
    ui/title_screen.cpp
    ui/quest_ui.cpp
    ui/text_renderer.cpp
    ui/image_renderer.cpp
    ui/debug_hud.cpp
    ui/settings_ui.cpp
    ui/save_load_ui.cpp

    # System
    system/settings_manager.cpp

    # Localization
    localization/localization_manager.cpp

    # Profiling
    profiling/performance_monitor.cpp

    # Save System
    save_system/save_manager.cpp
    save_system/save_system.cpp
    save_system/save_validator.cpp

    # Asset System
    assets/nif_parser.cpp
    assets/dds_loader.cpp
    assets/asset_manager.cpp
    assets/bsa_parser.cpp

    # JNI Bridge (Java/C++ communication)
    jni_bridge.cpp

    # Test Suite (Phase 4E)
    test/save_system_test.cpp
)

# Include directories
target_include_directories(native-lib PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/engine
    ${CMAKE_CURRENT_SOURCE_DIR}/game
    ${CMAKE_CURRENT_SOURCE_DIR}/world
    ${CMAKE_CURRENT_SOURCE_DIR}/ui
    ${CMAKE_CURRENT_SOURCE_DIR}/geometry
    ${CMAKE_CURRENT_SOURCE_DIR}/localization
    ${CMAKE_CURRENT_SOURCE_DIR}/profiling
    ${CMAKE_CURRENT_SOURCE_DIR}/save_system
    ${CMAKE_CURRENT_SOURCE_DIR}/system
    ${CMAKE_CURRENT_SOURCE_DIR}/audio
    ${CMAKE_CURRENT_SOURCE_DIR}/assets
    ${CMAKE_CURRENT_SOURCE_DIR}/test
    ${CMAKE_CURRENT_SOURCE_DIR}/external
    ${CMAKE_CURRENT_SOURCE_DIR}/external/glm
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
