# Project Info
cmake_minimum_required(VERSION 3.6 FATAL_ERROR)
cmake_policy(VERSION 3.6)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
project(HelloTriangle
        VERSION 1.0.0.0
        LANGUAGES C CXX
        )
enable_language(C)
enable_language(CXX)

# CMake Settings
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(CMAKE_SUPPRESS_REGENERATION true)
set(DCMAKE_GENERATOR_PLATFORM "x64")
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin)
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin)
SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin)
SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin)
SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin)
SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin)
if(NOT CMAKE_DEBUG_POSTFIX)
  set(CMAKE_DEBUG_POSTFIX d)
endif()

# Options
set(XGFX_PROTOCOL VULKAN CACHE STRING "Which graphics API to use, defaults to AUTO, can be NOOP, VULKAN, OPENGL, DIRECTX12, or METAL.")
set_property(
    CACHE
    XGFX_PROTOCOL PROPERTY
    STRINGS NOOP VULKAN OPENGL DIRECTX12 METAL
)
# =============================================================

# Core Dependencies
add_subdirectory(../../external/crosswindow ${CMAKE_BINARY_DIR}/crosswindow)
add_subdirectory(../../external/crosswindow-graphics ${CMAKE_BINARY_DIR}/crosswindow-graphics)

if(XGFX_PROTOCOL STREQUAL "VULKAN")
    find_path(VULKAN_INCLUDE_DIR NAMES vulkan/vulkan.h HINTS
        "$ENV{VULKAN_SDK}/include"
        "$ENV{VULKAN_SDK}/Include"
        "$ENV{VK_SDK_PATH}/Include")
    if (CMAKE_SIZEOF_VOID_P EQUAL 8)
        find_library(VULKAN_LIBRARY
            NAMES vulkan-1 vulkan vulkan.1
            HINTS
            "$ENV{VULKAN_SDK}/lib"
            "$ENV{VULKAN_SDK}/Lib"
            "$ENV{VULKAN_SDK}/Bin"
            "$ENV{VK_SDK_PATH}/Bin")
    else()
        find_library(VULKAN_LIBRARY
                    NAMES vulkan-1 vulkan vulkan.1
                    HINTS
            "$ENV{VULKAN_SDK}/Lib32"
            "$ENV{VULKAN_SDK}/Bin32"
            "$ENV{VK_SDK_PATH}/Bin32")
    endif()
endif()

set_property(TARGET CrossWindow PROPERTY FOLDER "Dependencies")

# =============================================================

# Sources

file(GLOB_RECURSE FILE_SOURCES RELATIVE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src/XMain.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/${XGFX_PROTOCOL}Renderer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/${XGFX_PROTOCOL}Renderer.mm
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h
)

# Solution Filters
foreach(source IN LISTS FILE_SOURCES)
    get_filename_component(source_path "${source}" PATH)
    string(REPLACE "/" "\\" source_path_msvc "${source_path}")
    string(REPLACE "src" "" source_path_final "${source_path_msvc}")
    source_group("${source_path_final}" FILES "${source}")
endforeach()

# =============================================================

# Finalize App
xwin_add_executable(
    ${PROJECT_NAME}
    "${FILE_SOURCES}"
)
# =============================================================

# Finish Dependencies

target_link_libraries(
    ${PROJECT_NAME}
    ${VULKAN_LIBRARY}
    CrossWindowGraphics
    CrossWindow
)

target_include_directories(
  ${PROJECT_NAME}
  PUBLIC "../../external/vectormath"
  PUBLIC ${VULKAN_INCLUDE_DIR}
)

target_compile_definitions(
  ${PROJECT_NAME} 
  PRIVATE XGFX_${XGFX_PROTOCOL}=1
)

# Finish Settings
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# =============================================================

# Preprocessor Definitions
target_compile_definitions(${PROJECT_NAME} PUBLIC XGFX_${XGFX_PROTOCOL}=1)