cmake_minimum_required(VERSION 3.10)
project(caneta-macos VERSION 1.0.0 LANGUAGES C CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Find required packages
find_package(SDL2 REQUIRED)

# Check if we're part of the parent project or standalone
if(TARGET caneta-sdl)
  message(STATUS "Using caneta-sdl from parent project")
  set(CANETA_SDL_FOUND TRUE)
else()
  # Try to find caneta-sdl in the parent directory
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../libraries/caneta-sdl")
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../libraries/caneta-sdl
      ${CMAKE_CURRENT_BINARY_DIR}/caneta-sdl)
    set(CANETA_SDL_FOUND TRUE)
  else()
    message(FATAL_ERROR "caneta-sdl not found")
  endif()
endif()

# Check for caneta-c
if(TARGET caneta-c)
  message(STATUS "Using caneta-c from parent project")
  set(CANETA_C_FOUND TRUE)
else()
  # Try to find caneta-c
  set(CANETA_C_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../libraries/caneta-c" CACHE PATH "Path to caneta-c")
  if(EXISTS "${CANETA_C_DIR}/caneta.h")
    set(CANETA_C_FOUND TRUE)
    set(CANETA_C_INCLUDE_DIR ${CANETA_C_DIR})
    set(CANETA_C_SOURCE "${CANETA_C_DIR}/caneta.c")
  else()
    message(FATAL_ERROR "caneta-c not found at ${CANETA_C_DIR}")
  endif()
endif()

# Create the test executable
add_executable(caneta-macos-test
  src/main.cpp
  src/key_logger.cpp
  src/key_logger.h
)

# If caneta-c is not a target, compile its source directly
if(NOT TARGET caneta-c AND CANETA_C_FOUND)
  target_sources(caneta-macos-test PRIVATE ${CANETA_C_SOURCE})
  target_include_directories(caneta-macos-test PRIVATE ${CANETA_C_INCLUDE_DIR})
endif()

# Include directories
target_include_directories(caneta-macos-test PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${CANETA_LIB_SOURCE_DIR}
  ${SDL2_INCLUDE_DIRS}
)

# Link libraries
target_link_libraries(caneta-macos-test
  caneta-sdl
  ${SDL2_LIBRARIES}
)

# If caneta-c is a target, link to it
if(TARGET caneta-c)
  target_link_libraries(caneta-macos-test caneta-c)
endif()

# macOS specific settings
if(APPLE)
  find_library(SDL2_FRAMEWORK SDL2)
  if(SDL2_FRAMEWORK)
    target_link_libraries(caneta-macos-test ${SDL2_FRAMEWORK})
  endif()

  # Link required macOS frameworks
  target_link_libraries(caneta-macos-test
    "-framework Cocoa"
    "-framework IOKit"
    "-framework CoreVideo"
  )

  # Set rpath for finding libraries
  set_target_properties(caneta-macos-test PROPERTIES
    INSTALL_RPATH "@executable_path/../lib"
    BUILD_WITH_INSTALL_RPATH TRUE
  )
endif()

# Set output directory
set_target_properties(caneta-macos-test PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
)

# Print configuration
message(STATUS "")
message(STATUS "caneta-macos-test configuration:")
message(STATUS "  SDL2: ${SDL2_LIBRARIES}")
message(STATUS "  caneta-sdl: ${CANETA_SDL_FOUND}")
message(STATUS "  caneta-c: ${CANETA_C_FOUND}")
message(STATUS "")