cmake_minimum_required(VERSION 3.15)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
    message(FATAL_ERROR "In-tree builds are not supported. Run CMake from a separate directory: cmake -B build")
endif()

project(AIDbg VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Folder support
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Fix warnings about DOWNLOAD_EXTRACT_TIMESTAMP
if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()

# Static runtime (must match x64dbg build)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/msvc-static-runtime.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/msvc-configurations.cmake)

# Allow user to override SDK path
option(AIDBG_USE_LOCAL_SDK "Use locally installed x64dbg SDK instead of fetching" OFF)
set(X64DBG_SDK_DIR "" CACHE PATH "Path to x64dbg pluginsdk directory (optional)")

if(NOT AIDBG_USE_LOCAL_SDK AND NOT X64DBG_SDK_DIR)
    include(FetchContent)
    message(STATUS "Fetching x64dbg SDK from SourceForge...")
    FetchContent_Declare(x64dbg
        URL "https://downloads.sourceforge.net/project/x64dbg/snapshots/snapshot_2023-06-10_18-05.zip"
        URL_HASH SHA1=04468bd61fb36d6b10d17f342f03ef12f5b2ce62
        TIMEOUT 120
    )
    FetchContent_MakeAvailable(x64dbg)
endif()

# Include x64dbg helper
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/x64dbg.cmake)

set(AIDbg_SOURCES
    src/pluginmain.cpp
    src/plugin.cpp
    src/utils/HttpClient.cpp
    src/utils/ConfigManager.cpp
    src/llm/ProviderRegistry.cpp
    src/llm/GLMProvider.cpp
    src/llm/OpenAIProvider.cpp
    src/llm/OllamaProvider.cpp
    src/core/ContextCollector.cpp
    src/commands/AICommands.cpp
    src/ui/SettingsDialog.cpp
)

x64dbg_plugin(AIDbg)

target_sources(AIDbg PRIVATE ${AIDbg_SOURCES})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${AIDbg_SOURCES})

# Copy prompts to output directory
add_custom_command(TARGET AIDbg POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/prompts
        $<TARGET_FILE_DIR:AIDbg>/prompts
    COMMENT "Copying prompt templates..."
)
