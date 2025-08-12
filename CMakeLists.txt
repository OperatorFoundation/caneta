cmake_minimum_required(VERSION 3.20)

# Detect build platform and target
if(APPLE OR UNIX AND NOT PICO_PLATFORM)
  set(BUILD_FOR_HOST ON)
  message(STATUS "Building for host platform (macOS/Linux)")
elseif(PICO_PLATFORM OR PICO_SDK_PATH)
  set(BUILD_FOR_PICO ON)
  message(STATUS "Building for Raspberry Pi Pico")
endif()

# Handle Pico SDK setup only when building for Pico
if(BUILD_FOR_PICO)
  set(PICO_SDK_PATH ${CMAKE_SOURCE_DIR}/libraries/pico-sdk CACHE PATH "Path to Pico SDK")
  set(PIO_USB_PATH ${CMAKE_SOURCE_DIR}/libraries/pio-usb CACHE PATH "Path to Pico-PIO-USB")

  # Include Pico SDK - must be before project()
  include(${PICO_SDK_PATH}/external/pico_sdk_import.cmake)
endif()

# Project declaration
project(caneta C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Platform-specific initialization
if(BUILD_FOR_PICO)
  # Initialize the Pico SDK
  pico_sdk_init()

  # Print Pico configuration
  message(STATUS "")
  message(STATUS "Caneta build configuration for Pico:")
  message(STATUS "  PICO_SDK_PATH: ${PICO_SDK_PATH}")
  message(STATUS "  PIO_USB_PATH: ${PIO_USB_PATH}")
  message(STATUS "")

  # Include platform-specific modules
  include(arduino-cli.cmake)
  include(esp32.cmake)
elseif(BUILD_FOR_HOST)
  # Host platform configuration
  message(STATUS "")
  message(STATUS "Caneta build configuration for host:")
  message(STATUS "  Platform: ${CMAKE_SYSTEM_NAME}")
  message(STATUS "  Compiler: ${CMAKE_CXX_COMPILER_ID}")
  message(STATUS "")
endif()

# Always build caneta-c (cross-platform C library)
add_subdirectory("libraries/caneta-c")

# Conditionally build platform-specific libraries
if(BUILD_FOR_PICO)
  # RP2040-specific build
  if(EXISTS "${CMAKE_SOURCE_DIR}/libraries/caneta-rp2040/CMakeLists.txt")
    add_subdirectory("libraries/caneta-rp2040")
  endif()

  # ESP32-specific build (if applicable)
  if(EXISTS "${CMAKE_SOURCE_DIR}/libraries/caneta-esp32/CMakeLists.txt")
    add_subdirectory("libraries/caneta-esp32")
  endif()
elseif(BUILD_FOR_HOST)
  # SDL build for macOS/Linux
  if(EXISTS "${CMAKE_SOURCE_DIR}/libraries/caneta-sdl/CMakeLists.txt")
    add_subdirectory("libraries/caneta-sdl")
  else()
    message(WARNING "caneta-sdl directory not found")
  endif()

  # Add caneta-macos test program if it exists
  if(EXISTS "${CMAKE_SOURCE_DIR}/caneta-macos/CMakeLists.txt")
    add_subdirectory("caneta-macos")
  endif()
endif()

# Option to build all libraries (for CI/testing)
option(BUILD_ALL_PLATFORMS "Attempt to build all platform libraries" OFF)
if(BUILD_ALL_PLATFORMS)
  message(WARNING "BUILD_ALL_PLATFORMS is experimental and may fail on some platforms")
  # Try to build everything that exists
  foreach(subdir caneta-rp2040 caneta-esp32 caneta-sdl)
    if(EXISTS "${CMAKE_SOURCE_DIR}/libraries/${subdir}/CMakeLists.txt")
      add_subdirectory("libraries/${subdir}")
    endif()
  endforeach()
endif()