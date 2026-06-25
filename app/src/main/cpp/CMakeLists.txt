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
    engine/texture_loader.cpp
    engine/graphics/retro_filter.cpp

    # Geometry
    geometry/cube.cpp
    geometry/mesh.cpp
    geometry/material.cpp

    # World System (Phase 3+)
    world/world_manager.cpp
    world/world_item_manager.cpp
    world/cell.cpp
    world/cell_loader.cpp
    world/cell_transition_manager.cpp
    world/door.cpp
    world/grid.cpp
    world/terrain.cpp

    # Game Systems
    game/npc.cpp
    game/npc_manager.cpp
    game/combat_manager.cpp
    game/quest_manager.cpp
    game/spell_manager.cpp
    game/dialogue.cpp
    game/door.cpp
    game/interactable.cpp
    game/container.cpp
    game/interaction_manager.cpp
    game/merchant.cpp

    # Player & Movement (Phase 3+)
    game/player.cpp
    game/player_controller.cpp
    input/touch_input_handler.cpp

    # Inventory System (Phase 3+)
    game/inventory.cpp
    game/inventory_manager.cpp
    game/consumable_system.cpp
    game/equipment_effect_system.cpp
    game/book_database.cpp

    # UI Systems
    ui/title_screen.cpp
    ui/quest_ui.cpp
    ui/inventory_ui.cpp
    ui/text_renderer.cpp
    ui/debug_hud.cpp
    ui/settings_ui.cpp
    ui/save_load_ui.cpp
    ui/ui_component.cpp
    ui/ui_system.cpp
    ui/ui_panel.cpp
    ui/ui_button.cpp
    ui/ui_draw_helper.cpp
    ui/ui_map_panel.cpp
    ui/ui_inventory_panel.cpp
    ui/ui_character_sheet.cpp
    ui/ui_spellbook.cpp
    ui/ui_quest_log.cpp
    ui/ui_dialogue.cpp
    ui/ui_shop.cpp
    ui/ui_character_creation.cpp
    ui/ui_pause_menu.cpp
    ui/ui_manager.cpp
    ui/placeholder_assets.cpp
    ui/hud_renderer.cpp

    # Map System (Phase 9.1)
    map/map_system.cpp

    # Inventory System (Phase 9.2)
    inventory/inventory_grid.cpp
    inventory/equipment_manager.cpp
    inventory/item_factory.cpp
    inventory/inventory_coordinator.cpp

    # System
    system/settings_manager.cpp

    # Localization
    localization/localization_manager.cpp

    # Profiling
    profiling/performance_monitor.cpp

    # Save System
    save_system/save_manager.cpp

    # Audio System (Phase 8+)
    audio/audio_manager.cpp
    audio/audio_3d.cpp
    audio/jni_audio_bridge.cpp

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
    ${CMAKE_CURRENT_SOURCE_DIR}/map
    ${CMAKE_CURRENT_SOURCE_DIR}/inventory
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

# Enable audio system (OpenAL stub headers used)
target_compile_definitions(native-lib PRIVATE AUDIO_SYSTEM_ENABLED)

# Warning suppression for specific headers
set_source_files_properties(
    game/npc_manager.cpp
    game/quest_manager.cpp
    PROPERTIES
    COMPILE_FLAGS "-Wno-unused-variable"
)
