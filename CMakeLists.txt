cmake_minimum_required(VERSION 3.28)

# ==============================================================================
# Project Setup
# ==============================================================================
project(Midgard
    VERSION 0.1.0
    DESCRIPTION "Modern C++20 3D Rendering Engine with Vulkan & OpenGL"
    LANGUAGES C CXX
)

# Enforce C++20 standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Default to Debug build type if not specified (for Ninja, etc.)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Choose the type of build." FORCE)
endif()

# Export compilation database for LSP / IDE IntelliSense
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Output binaries and DLLs to bin/
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}/bin/Debug")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/bin/Release")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")

# ==============================================================================
# Dependencies (via vcpkg)
# ==============================================================================

# 1. Windowing & Graphics APIs
message(STATUS "[Midgard] Finding GLFW3...")
find_package(glfw3 CONFIG REQUIRED)

message(STATUS "[Midgard] Finding OpenGL...")
find_package(OpenGL REQUIRED)

message(STATUS "[Midgard] Finding GLAD...")
find_package(glad CONFIG REQUIRED)

message(STATUS "[Midgard] Finding Vulkan...")
find_package(Vulkan REQUIRED)

# 2. Math, Images & 3D Assets
message(STATUS "[Midgard] Finding GLM...")
find_package(glm CONFIG REQUIRED)

message(STATUS "[Midgard] Finding stb...")
find_package(stb CONFIG QUIET)
if(NOT TARGET stb::stb)
    find_package(Stb REQUIRED)
    add_library(stb::stb INTERFACE IMPORTED)
    target_include_directories(stb::stb INTERFACE ${Stb_INCLUDE_DIR})
endif()

message(STATUS "[Midgard] Finding Assimp...")
find_package(assimp CONFIG REQUIRED)

# 3. Logging & UI
message(STATUS "[Midgard] Finding spdlog...")
find_package(spdlog CONFIG REQUIRED)

message(STATUS "[Midgard] Finding Dear ImGui...")
find_package(imgui CONFIG REQUIRED)

# 4. Vulkan Utilities & Shaders
message(STATUS "[Midgard] Finding vk-bootstrap...")
find_package(vk-bootstrap CONFIG REQUIRED)

# Shaderc (vcpkg unofficial-shaderc or Vulkan SDK fallback)
find_package(unofficial-shaderc CONFIG QUIET)
if(TARGET unofficial::shaderc::shaderc)
    add_library(shaderc_target INTERFACE)
    target_link_libraries(shaderc_target INTERFACE unofficial::shaderc::shaderc)
elseif(TARGET unofficial::shaderc::shaderc_shared)
    add_library(shaderc_target INTERFACE)
    target_link_libraries(shaderc_target INTERFACE unofficial::shaderc::shaderc_shared)
elseif(TARGET unofficial::shaderc::shaderc_combined)
    add_library(shaderc_target INTERFACE)
    target_link_libraries(shaderc_target INTERFACE unofficial::shaderc::shaderc_combined)
else()
    find_library(SHADERC_LIB NAMES shaderc_shared shaderc_combined shaderc HINTS $ENV{VULKAN_SDK}/lib)
    if(SHADERC_LIB)
        message(STATUS "[Midgard] Found shaderc library from Vulkan SDK: ${SHADERC_LIB}")
        add_library(shaderc_target INTERFACE)
        target_link_libraries(shaderc_target INTERFACE "${SHADERC_LIB}")
        target_include_directories(shaderc_target INTERFACE "${Vulkan_INCLUDE_DIRS}")
    else()
        message(WARNING "[Midgard] shaderc library not found, shader compilation at runtime might be unavailable.")
        add_library(shaderc_target INTERFACE)
    endif()
endif()

# 5. HLSL & SPIR-V Pipeline (DXC, SPIRV-Cross, SPIRV-Tools)
message(STATUS "[Midgard] Finding DirectX Shader Compiler (DXC)...")
find_package(directx-dxc CONFIG REQUIRED)

message(STATUS "[Midgard] Finding SPIRV-Cross...")
find_package(spirv_cross_core CONFIG REQUIRED)
find_package(spirv_cross_glsl CONFIG REQUIRED)

message(STATUS "[Midgard] Finding SPIRV-Tools...")
find_package(SPIRV-Tools CONFIG REQUIRED)
find_package(SPIRV-Tools-opt CONFIG REQUIRED)

# ==============================================================================
# Engine Library & Executable
# ==============================================================================

# Collect engine sources and headers
file(GLOB_RECURSE ENGINE_SOURCES CONFIGURE_DEPENDS
    "src/*.cpp" 
    "src/*.c" 
    "src/*.h" 
    "src/*.hpp"
)

# Static library for the engine
add_library(MidgardEngine STATIC
    ${ENGINE_SOURCES}
)

# Main executable
add_executable(Midgard
    main.cpp
)

target_link_libraries(Midgard PRIVATE
    MidgardEngine
)

# Include directories
target_include_directories(MidgardEngine PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Precompiled Headers (PCH)
target_precompile_headers(MidgardEngine PUBLIC
    <vector>
    <string>
    <memory>
    <unordered_map>
    <filesystem>
    <functional>
    <algorithm>
    <cstdint>
    <glm/glm.hpp>
    <spdlog/spdlog.h>
)

# Compiler options
if(MSVC)
    target_compile_options(MidgardEngine PUBLIC 
        /W4 
        /utf-8
        /permissive-
        /external:anglebrackets
        /external:W0
        /MP
    )
else()
    target_compile_options(MidgardEngine PUBLIC 
        -Wall 
        -Wextra 
        -Wpedantic
    )
endif()

# Compiler definitions
target_compile_definitions(MidgardEngine PUBLIC
    GLM_FORCE_RADIANS
    GLM_FORCE_DEPTH_ZERO_TO_ONE
    GLFW_INCLUDE_NONE
    ASSETS_PATH="${CMAKE_CURRENT_SOURCE_DIR}/assets/"
    SPDLOG_ACTIVE_LEVEL=0
    $<$<OR:$<CONFIG:Debug>,$<CONFIG:DEBUG>,$<CONFIG:debug>>:DEBUG>
)

if(WIN32)
    target_compile_definitions(MidgardEngine PUBLIC
        NOMINMAX
        WIN32_LEAN_AND_MEAN
    )
endif()

# Link dependencies
target_link_libraries(MidgardEngine PUBLIC
    glfw
    OpenGL::GL
    glad::glad
    Vulkan::Vulkan
    glm::glm
    stb::stb
    assimp::assimp
    spdlog::spdlog
    imgui::imgui
    vk-bootstrap::vk-bootstrap
    shaderc_target
    Microsoft::DirectXShaderCompiler
    spirv-cross-glsl
    spirv-cross-core
    SPIRV-Tools-opt
    SPIRV-Tools-static
)

# Clang-Tidy static analysis option
option(MIDGARD_ENABLE_CLANG_TIDY "Enable clang-tidy static analysis during compilation" OFF)
if(MIDGARD_ENABLE_CLANG_TIDY)
    find_program(CLANG_TIDY_EXE 
        NAMES clang-tidy
        HINTS 
            "$ENV{VCINSTALLDIR}Tools/Llvm/x64/bin"
            "C:/Program Files/Microsoft Visual Studio/18/Community/VC/Tools/Llvm/x64/bin"
    )
    if(CLANG_TIDY_EXE)
        message(STATUS "[Midgard] Clang-Tidy enabled: ${CLANG_TIDY_EXE}")
        set_target_properties(MidgardEngine PROPERTIES CXX_CLANG_TIDY "${CLANG_TIDY_EXE}")
        set_target_properties(Midgard PROPERTIES CXX_CLANG_TIDY "${CLANG_TIDY_EXE}")
    else()
        message(WARNING "[Midgard] Clang-Tidy requested but executable was not found.")
    endif()
endif()

# Copie automatique de compile_commands.json a la racine pour l'IDE / LSP
# ATTENTION : Ne JAMAIS utiliser configure_file() sur compile_commands.json.
# configure_file() enregistre le fichier comme dependance d'entree de build.ninja,
# ce qui provoque une boucle infinie de reconfiguration (manifest dirty after 100 tries).
if(CMAKE_EXPORT_COMPILE_COMMANDS)
    add_custom_target(copy_compile_commands ALL
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_BINARY_DIR}/compile_commands.json"
            "${CMAKE_SOURCE_DIR}/compile_commands.json"
        COMMENT "[Midgard] Syncing compile_commands.json to workspace root..."
        VERBATIM
    )
endif()
