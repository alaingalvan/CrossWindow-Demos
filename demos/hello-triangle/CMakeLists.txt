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

# Core Dependencies
add_subdirectory(../../external/crosswindow)

target_include_directories(
  ${PROJECT_NAME}
  PRIVATE ../../exteral/crosswindow-graphics/src
)

target_compile_definitions(
  ${PROJECT_NAME} 
  PRIVATE XGFX_VULKAN=1
)

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


# =============================================================

# Sources

file(GLOB_RECURSE FILE_SOURCES RELATIVE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.mm
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

set_property(TARGET CrossWindow  PROPERTY FOLDER "Dependencies")

# Finalize Engine Editor
xwin_add_executable(
    ${PROJECT_NAME}
    "${FILE_SOURCES}"
)
	
# Finish Dependencies
target_link_libraries(
    ${PROJECT_NAME}
    ${VULKAN_LIBRARY}
)

target_include_directories(${PROJECT_NAME} PUBLIC ${VULKAN_INCLUDE_DIR})

# Finish Settings
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)