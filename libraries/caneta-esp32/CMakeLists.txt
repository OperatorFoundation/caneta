cmake_minimum_required(VERSION 3.31)

set(CMAKE_CXX_STANDARD 17)
set(CANETA_LIB_DIR ${CMAKE_SOURCE_DIR}/libraries/caneta-c)
set(CANETA_LIB_SRC_DIR ${CMAKE_SOURCE_DIR}/src)
set(CANETA_ESP32_DIR ${CMAKE_SOURCE_DIR}/libraries/caneta-esp32)
set(CANETA_ESP32_SRC_DIR ${CANETA_ESP32_DIR}/src)
set(CANETA_ESP32_EXAMPLE_DIR ${CANETA_ESP32_DIR}/examples/caneta-esp32)
include_directories(${CANETA_LIB_SRC_DIR} ${CANETA_ESP32_SRC_DIR})

# Define your sketch file
set(SKETCH_FILE ${CANETA_ESP32_EXAMPLE_DIR}/caneta-esp32.ino)

set(BOARD "esp32:esp32:adafruit_feather_esp32s3_nopsram")
set(PORT "/dev/cu.usbmodem1101")

set(OUTPUT_DIR ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/esp32/s3/caneta)
file(GLOB_RECURSE CANETA_FILES ${CANETA_LIB_SRC_DIR}/*.cpp ${CANETA_ESP32_SRC_DIR}/*.cpp ${CANETA_ESP32_EXAMPLE_DIR}/*.cpp ${CANETA_ESP32_EXAMPLE_DIR}/*.ino)

add_executable(caneta-esp32 ${CANETA_FILES})
add_dependencies(caneta-esp32 build_caneta_esp32)

add_custom_target(build_caneta_esp32 ALL
        COMMAND arduino-cli compile
            --fqbn ${BOARD}
            --output-dir ${OUTPUT_DIR}
            --build-property "build.extra_flags=-I${CMAKE_SOURCE_DIR}"
            --build-property "compiler.cpp.extra_flags=-std=c++17"
            --library ${CANETA_LIB_DIR}
            --library ${CANETA_ESP32_DIR}
            ${SKETCH_FILE}
        WORKING_DIRECTORY ${CANETA_ESP32_SRC_DIR}
        COMMENT "Building caneta-esp32 with arduino-cli"
)

add_custom_target(upload_caneta_esp32 ALL
        COMMAND arduino-cli upload
        -p ${PORT}
        --fqbn ${BOARD}
        --output-dir ${OUTPUT_DIR}
        --build-property "build.extra_flags=-I${CMAKE_SOURCE_DIR}"
        --build-property "compiler.cpp.extra_flags=-std=c++17"
        --library ${CANETA_LIB_DIR}
        --library ${CANETA_ESP32_DIR}
        ${SKETCH_FILE}
        WORKING_DIRECTORY ${CANETA_ESP32_SRC_DIR}
        COMMENT "Uploading caneta-esp32 with arduino-cli"
)