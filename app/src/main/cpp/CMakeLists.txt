cmake_minimum_required(VERSION 3.16.0)
project("oblivion_android" VERSION 0.1.0 LANGUAGES CXX C)

# C++17標準
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Android NDK設定
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O2")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -DDEBUG -DENABLE_DEBUG_LOGS")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -fvisibility=hidden")

# Vulkanパッケージ検出
find_package(Vulkan REQUIRED)

# ヘッダー検索パス
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/engine
    ${CMAKE_CURRENT_SOURCE_DIR}/engine/graphics
    ${CMAKE_CURRENT_SOURCE_DIR}/game
    ${CMAKE_CURRENT_SOURCE_DIR}/world
    ${CMAKE_CURRENT_SOURCE_DIR}/input
    ${CMAKE_CURRENT_SOURCE_DIR}/util
    ${CMAKE_CURRENT_SOURCE_DIR}/ui
    ${CMAKE_CURRENT_SOURCE_DIR}/audio
    ${CMAKE_CURRENT_SOURCE_DIR}/jni
    ${Vulkan_INCLUDE_DIRS}
)

# ソースファイル収集
file(GLOB_RECURSE ENGINE_SOURCES
    "engine/*.cpp"
    "engine/graphics/*.cpp"
)

file(GLOB_RECURSE GAME_SOURCES
    "game/*.cpp"
    "world/*.cpp"
)

file(GLOB_RECURSE SYSTEM_SOURCES
    "input/*.cpp"
    "ui/*.cpp"
    "audio/*.cpp"
)

file(GLOB_RECURSE JNI_SOURCES
    "jni/*.cpp"
)

# メインライブラリ
add_library(oblivion_engine SHARED
    ${ENGINE_SOURCES}
    ${GAME_SOURCES}
    ${SYSTEM_SOURCES}
    ${JNI_SOURCES}
)

# リンクライブラリ
target_link_libraries(oblivion_engine
    android
    log
    EGL
    GLESv3
    ${Vulkan_LIBRARIES}
)

# コンパイル定義
target_compile_definitions(oblivion_engine PRIVATE
    VULKAN_HPP_NO_EXCEPTIONS
    VULKAN_HPP_ASSERT_ON_RESULT
    ANDROID_PLATFORM
)
