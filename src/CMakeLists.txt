cmake_minimum_required(VERSION 4.1.1)
project(jamExample)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

add_library(jamPlatform STATIC IMPORTED)
add_executable(${PROJECT_NAME} ${CMAKE_SOURCE_DIR}/main.cpp)

if (WIN32)
  message("Building for windows")
  set_target_properties(${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_NAME "${PROJECT_NAME}.exe")
  set_target_properties(jamPlatform PROPERTIES
      IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/jamPlatform.lib"
      INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/jamPlatforms/"
  )
endif()

if (UNIX)
  message("Building for linux")
  set_target_properties(${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_NAME "${PROJECT_NAME}.out")
  set_target_properties(jamPlatform PROPERTIES
    IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/jamPlatform.a"
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/jamPlatforms/"
  )
endif()

target_link_libraries(${PROJECT_NAME} jamPlatform)

