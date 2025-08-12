cmake_minimum_required(VERSION 3.10)

# Only set project if not already in a parent project
if(NOT PROJECT_NAME)
  project(caneta-sdl VERSION 1.0.0 LANGUAGES C CXX)
endif()

# Set C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Option to build examples
option(CANETA_SDL_BUILD_EXAMPLES "Build caneta-sdl example programs" ON)

# Find required packages
find_package(SDL2 QUIET)

# If SDL2 not found via find_package, try pkg-config
if(NOT SDL2_FOUND)
  find_package(PkgConfig QUIET)
  if(PKG_CONFIG_FOUND)
    pkg_check_modules(SDL2 sdl2)
  endif()
endif()

# If still not found, look for SDL2 manually on macOS
if(NOT SDL2_FOUND AND APPLE)
  # Check for SDL2 framework
  find_library(SDL2_FRAMEWORK SDL2
    PATHS
    /Library/Frameworks
    ~/Library/Frameworks
    /System/Library/Frameworks
  )

  if(SDL2_FRAMEWORK)
    set(SDL2_FOUND TRUE)
    set(SDL2_INCLUDE_DIRS ${SDL2_FRAMEWORK}/Headers)
    set(SDL2_LIBRARIES ${SDL2_FRAMEWORK})
    message(STATUS "Found SDL2 framework: ${SDL2_FRAMEWORK}")
  endif()
endif()

# Final check for SDL2
if(NOT SDL2_FOUND)
  message(FATAL_ERROR "SDL2 not found. Please install SDL2:\n"
    "  macOS: brew install sdl2\n"
    "  Ubuntu: apt-get install libsdl2-dev\n"
    "  Or download from https://www.libsdl.org/")
endif()

# Find caneta-c library
# First check if it's part of the parent project
if(TARGET caneta-c)
  message(STATUS "Using caneta-c from parent project")
  set(CANETA_C_FOUND TRUE)
  # Get the actual source directory from the target
  get_target_property(CANETA_C_SOURCE_DIR caneta-c SOURCE_DIR)
  if(CANETA_C_SOURCE_DIR)
    set(CANETA_C_INCLUDE_DIR ${CANETA_C_SOURCE_DIR})
  else()
    # Fallback to expected location
    set(CANETA_C_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/libraries/caneta-c)
  endif()
  message(STATUS "caneta-c include dir: ${CANETA_C_INCLUDE_DIR}")
else()
  # Otherwise look for it
  set(CANETA_C_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../caneta-c" CACHE PATH "Path to caneta-c library")

  if(EXISTS "${CANETA_C_DIR}/caneta.h")
    message(STATUS "Found caneta-c at: ${CANETA_C_DIR}")
    set(CANETA_C_FOUND TRUE)
    set(CANETA_C_INCLUDE_DIR ${CANETA_C_DIR})
  else()
    # Try other common locations
    find_path(CANETA_C_INCLUDE_DIR caneta.h
      HINTS
      ${CMAKE_SOURCE_DIR}/libraries/caneta-c
      ${CMAKE_CURRENT_SOURCE_DIR}/../caneta-c
      /usr/local/include
      /usr/include
    )

    if(CANETA_C_INCLUDE_DIR)
      set(CANETA_C_FOUND TRUE)
      message(STATUS "Found caneta-c at: ${CANETA_C_INCLUDE_DIR}")
    else()
      message(WARNING "caneta-c not found. Please set CANETA_C_DIR. Examples will not be built.")
      set(CANETA_C_FOUND FALSE)
    endif()
  endif()
endif()

# Create caneta-sdl library
add_library(caneta-sdl
  src/caneta_sdl.cpp
  src/caneta_sdl.h
)

# Set include directories
target_include_directories(caneta-sdl PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<INSTALL_INTERFACE:include>
)

# Add SDL2 includes
if(SDL2_INCLUDE_DIRS)
  target_include_directories(caneta-sdl PUBLIC ${SDL2_INCLUDE_DIRS})
endif()

# Add caneta-c includes if found
if(CANETA_C_FOUND)
  target_include_directories(caneta-sdl PUBLIC ${CANETA_C_INCLUDE_DIR})
endif()

# Link SDL2
if(TARGET SDL2::SDL2)
  target_link_libraries(caneta-sdl PUBLIC SDL2::SDL2)
elseif(SDL2_LIBRARIES)
  target_link_libraries(caneta-sdl PUBLIC ${SDL2_LIBRARIES})
endif()

# Link caneta-c if it's a target (IMPORTANT: This was missing!)
if(TARGET caneta-c)
  target_link_libraries(caneta-sdl PUBLIC caneta-c)
endif()

# macOS-specific settings
if(APPLE)
  # Ensure proper linking on macOS
  target_link_libraries(caneta-sdl PUBLIC
    "-framework Cocoa"
    "-framework IOKit"
    "-framework CoreVideo"
  )
endif()

# Export compile commands for IDEs like CLion
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Build examples if requested and caneta-c is found
if(CANETA_SDL_BUILD_EXAMPLES AND CANETA_C_FOUND)
  # Look for caneta.c source file
  set(CANETA_C_SOURCE "")

  if(TARGET caneta-c)
    # If caneta-c is a target, we can link to it
    set(CANETA_C_SOURCE "USE_TARGET")
  elseif(EXISTS "${CANETA_C_INCLUDE_DIR}/caneta.c")
    set(CANETA_C_SOURCE "${CANETA_C_INCLUDE_DIR}/caneta.c")
  elseif(EXISTS "${CMAKE_SOURCE_DIR}/libraries/caneta-c/caneta.c")
    set(CANETA_C_SOURCE "${CMAKE_SOURCE_DIR}/libraries/caneta-c/caneta.c")
  endif()
endif()

# Installation rules (optional, only if being built standalone)
if(PROJECT_NAME STREQUAL "caneta-sdl")
  install(TARGETS caneta-sdl
    EXPORT caneta-sdl-targets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
  )

  install(FILES src/caneta_sdl.h
    DESTINATION include
  )
endif()

# Print configuration summary
message(STATUS "")
message(STATUS "caneta-sdl configuration:")
message(STATUS "  SDL2 found: ${SDL2_FOUND}")
message(STATUS "  SDL2 include: ${SDL2_INCLUDE_DIRS}")
message(STATUS "  SDL2 libraries: ${SDL2_LIBRARIES}")
message(STATUS "  caneta-c found: ${CANETA_C_FOUND}")
message(STATUS "  caneta-c include: ${CANETA_C_INCLUDE_DIR}")
message(STATUS "  Build examples: ${CANETA_SDL_BUILD_EXAMPLES}")
message(STATUS "")