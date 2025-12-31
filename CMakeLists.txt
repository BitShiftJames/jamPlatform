cmake_minimum_required(VERSION 4.1.1)
project(jamPlatform)

set(platform_sources)
set(PLATFORM_PATH "${CMAKE_SOURCE_DIR}/src/jamPlatforms")

if (WIN32)
  message("Building for windows...")
  list(APPEND platform_sources ${PLATFORM_PATH}/platform_win32.cpp)
endif()

if (LINUX)
  message("Building for linux...")
  list(APPEND platform_sources ${PLATFORM_PATH}/platform_linux.cpp)
endif()

if (APPLE)
  message("Building for apple...")
  list(APPEND platform_sources ${PLATFORM_PATH}/platform_apple.cpp)
endif()

add_library(jamPlatform ${platform_sources})
set_target_properties(jamPlatform PROPERTIES
  PREFIX ""
  LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/src/
  LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/src/
  ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/src
  ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/src
)
