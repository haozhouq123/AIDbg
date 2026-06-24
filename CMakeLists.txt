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

# =============================================================================
# Inline: MSVC static runtime (must match x64dbg build)
# =============================================================================
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    foreach(flag_var
        CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
        CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO
        CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE
        CMAKE_C_FLAGS_MINSIZEREL CMAKE_C_FLAGS_RELWITHDEBINFO)
        if(${flag_var} MATCHES "/MD")
            string(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
        endif()
    endforeach()
endif()

# =============================================================================
# Inline: Default build configurations
# =============================================================================
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
        Debug Release MinSizeRel RelWithDebInfo)
endif()

# =============================================================================
# Inline: x64dbg SDK fetch
# =============================================================================
option(AIDBG_USE_LOCAL_SDK "Use locally installed x64dbg SDK instead of fetching" OFF)
set(X64DBG_SDK_DIR "" CACHE PATH "Path to x64dbg pluginsdk directory (optional)")

if(NOT AIDBG_USE_LOCAL_SDK AND NOT X64DBG_SDK_DIR)
    include(FetchContent)
    message(STATUS "Fetching x64dbg SDK from SourceForge...")
    FetchContent_Declare(x64dbg
        URL "https://downloads.sourceforge.net/project/x64dbg/snapshots/snapshot_2023-06-10_18-05.zip"
        URL_HASH SHA1=04468bd61fb36d6b10d17f342f03ef12f5b2ce62
        TIMEOUT 180
    )
    FetchContent_MakeAvailable(x64dbg)
endif()

# =============================================================================
# Inline: x64dbg_plugin() function
# =============================================================================
function(x64dbg_plugin target)
    # Determine architecture
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(ARCH "x64")
        set(SUFFIX ".dp64")
        set(LIB_NAME "x64dbg.lib")
    else()
        set(ARCH "x32")
        set(SUFFIX ".dp32")
        set(LIB_NAME "x32dbg.lib")
    endif()

    # Find SDK directory
    set(PLUGIN_SDK_DIR "")
    if(X64DBG_SDK_DIR AND EXISTS "${X64DBG_SDK_DIR}/bridgemain.h")
        set(PLUGIN_SDK_DIR "${X64DBG_SDK_DIR}")
    else()
        # Search common locations in fetched content
        foreach(candidate
            "${x64dbg_SOURCE_DIR}/pluginsdk"
            "${x64dbg_SOURCE_DIR}/release/pluginsdk"
            "${x64dbg_BINARY_DIR}/pluginsdk"
            "${x64dbg_BINARY_DIR}/release/pluginsdk"
        )
            if(EXISTS "${candidate}/bridgemain.h")
                set(PLUGIN_SDK_DIR "${candidate}")
                break()
            endif()
        endforeach()
    endif()

    if(NOT PLUGIN_SDK_DIR)
        message(FATAL_ERROR
            "x64dbg pluginsdk not found!\n"
            "Options:\n"
            "  1. Let CMake fetch it automatically (default, requires internet)\n"
            "  2. Set -DX64DBG_SDK_DIR=C:/path/to/x64dbg/pluginsdk\n"
            "  3. Download x64dbg snapshot from https://x64dbg.com\n"
        )
    endif()

    message(STATUS "Using x64dbg SDK: ${PLUGIN_SDK_DIR}")
    message(STATUS "SDK parent dir: ${PLUGIN_SDK_DIR}/..")
    message(STATUS "Building ${target} as ${ARCH} plugin (${SUFFIX})")
    
    # Diagnostic: list a few key files to confirm SDK is properly extracted
    foreach(check_file
        "${PLUGIN_SDK_DIR}/bridgemain.h"
        "${PLUGIN_SDK_DIR}/_plugins.h"
        "${PLUGIN_SDK_DIR}/${LIB_NAME}"
    )
        if(EXISTS "${check_file}")
            message(STATUS "  Found: ${check_file}")
        else()
            message(WARNING "  NOT FOUND: ${check_file}")
        endif()
    endforeach()

    # Find the .lib file
    set(LIB_PATH "")
    foreach(candidate
        "${PLUGIN_SDK_DIR}/lib/${LIB_NAME}"
        "${PLUGIN_SDK_DIR}/${LIB_NAME}"
        "${PLUGIN_SDK_DIR}/lib/${ARCH}/${LIB_NAME}"
    )
        if(EXISTS "${candidate}")
            set(LIB_PATH "${candidate}")
            break()
        endif()
    endforeach()

    if(NOT LIB_PATH)
        message(WARNING "x64dbg .lib not found at expected path. Will try to link anyway.")
        set(LIB_PATH "${PLUGIN_SDK_DIR}/lib/${LIB_NAME}")
    endif()

    # Create shared library
    add_library(${target} SHARED ${ARGN})

    # Include paths
    # Add BOTH the pluginsdk/ directory itself AND its parent.
    # - Parent dir: so #include "pluginsdk/bridgemain.h" works (used by PluginTemplate convention)
    # - pluginsdk/ dir: so #include "bridgemain.h" also works (defensive)
    get_filename_component(PLUGIN_SDK_PARENT "${PLUGIN_SDK_DIR}" DIRECTORY)
    target_include_directories(${target} PRIVATE
        "${PLUGIN_SDK_PARENT}"
        "${PLUGIN_SDK_DIR}"
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
    )

    # Link with x64dbg lib
    target_link_libraries(${target} PRIVATE "${LIB_PATH}")

    # Windows system libs (WinHTTP for HTTP, etc.)
    target_link_libraries(${target} PRIVATE
        winhttp
        wininet
        ws2_32
        user32
        gdi32
        comctl32
        advapi32
        crypt32
        shell32
        ole32
    )

    # Output naming: AIDbg.dp64 / AIDbg.dp32
    set_target_properties(${target} PROPERTIES
        PREFIX ""
        SUFFIX "${SUFFIX}"
        OUTPUT_NAME "${target}"
    )

    # C++ standard
    target_compile_features(${target} PRIVATE cxx_std_17)

    # Compile definitions
    target_compile_definitions(${target} PRIVATE
        _CRT_SECURE_NO_WARNINGS
        _WINSOCK_DEPRECATED_NO_WARNINGS
        NOMINMAX
        WIN32_LEAN_AND_MEAN
        UNICODE
        _UNICODE
        BUILD_PLUGIN
    )

    # Enable multicore compilation on MSVC
    if(MSVC)
        target_compile_options(${target} PRIVATE /MP /W3 /EHsc)
    endif()
endfunction()

# =============================================================================
# AIDbg plugin target
# =============================================================================
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

x64dbg_plugin(AIDbg ${AIDbg_SOURCES})

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${AIDbg_SOURCES})

# Copy prompts to output directory (optional, prompts are also embedded in code)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/prompts")
    add_custom_command(TARGET AIDbg POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/prompts
            $<TARGET_FILE_DIR:AIDbg>/prompts
        COMMENT "Copying prompt templates..."
    )
endif()
