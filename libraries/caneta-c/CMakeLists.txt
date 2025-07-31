cmake_minimum_required(VERSION 3.20)

# Set paths to external libraries
set(CANETA_C_PATH ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Project declaration
project(caneta-c C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Define target name
set(target_name caneta-c)

add_library(caneta-c STATIC
  ${CANETA_C_PATH}/caneta.c
  ../caneta-esp32/src/CanetaBluetooth.cpp
  ../caneta-esp32/src/CanetaBluetooth.h
)

target_include_directories(caneta-c PUBLIC
  ${CANETA_C_PATH}
)
