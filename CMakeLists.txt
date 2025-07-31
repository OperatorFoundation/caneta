cmake_minimum_required(VERSION 3.20)

set(PICO_SDK_PATH ${CMAKE_SOURCE_DIR}/libraries/pico-sdk CACHE PATH "Path to Pico SDK")
set(PIO_USB_PATH ${CMAKE_SOURCE_DIR}/libraries/pio-usb CACHE PATH "Path to Pico-PIO-USB")

# Include Pico SDK - must be before project()
include(${PICO_SDK_PATH}/external/pico_sdk_import.cmake)

# Project declaration
project(caneta C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)# Initialize the SDK

pico_sdk_init()

# Print configuration
message(STATUS "")
message(STATUS "Caneta build configuration:")
message(STATUS "  PICO_SDK_PATH: ${PICO_SDK_PATH}")
message(STATUS "  PIO_USB_PATH: ${PIO_USB_PATH}")
message(STATUS "")

include(arduino-cli.cmake)
include(esp32.cmake)

add_subdirectory("libraries/caneta-c")
add_subdirectory("libraries/caneta-rp2040")
add_subdirectory("libraries/caneta-esp32")