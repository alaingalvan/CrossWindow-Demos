cmake_minimum_required(VERSION 3.6 FATAL_ERROR)
cmake_policy(VERSION 3.6)
project(Glad
        VERSION 1.0.0.0
        LANGUAGES C CXX
)

set(
  GLAD_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/src/glad.c
)

add_library(
  ${PROJECT_NAME}
  "${GLAD_SOURCES}"
)

target_include_directories(
  Glad
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)