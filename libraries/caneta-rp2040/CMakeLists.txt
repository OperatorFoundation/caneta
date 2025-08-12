cmake_minimum_required(VERSION 3.20)

# Set paths to external libraries
set(CANETA_C_PATH ${CMAKE_SOURCE_DIR}/libraries/caneta-c/src)
set(CANETA_RP2040_PATH ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Define target name
set(target_name caneta-rp2040)
set(OUTPUT_DIR ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/rp2040/${target_name})
set(OUTPUT_FILE ${CMAKE_BINARY_DIR}/${target_name}.uf2)

# Compile PIO-USB so that we can link against it
file(GLOB_RECURSE PIO_USB_SOURCES "${PIO_USB_PATH}/src/*.c")
list(APPEND PIO_USB_SOURCES
  "${PICO_SDK_PATH}/lib/tinyusb/src/portable/raspberrypi/pio_usb/dcd_pio_usb.c"
  "${PICO_SDK_PATH}/lib/tinyusb/src/portable/raspberrypi/pio_usb/hcd_pio_usb.c"
)
add_library(pico_pio_usb STATIC ${PIO_USB_SOURCES})
target_include_directories(pico_pio_usb PUBLIC
  ${PIO_USB_PATH}/src
  ${CANETA_RP2040_PATH}
)
target_link_libraries(pico_pio_usb PUBLIC
  pico_stdlib
  hardware_pio
  hardware_dma
  pico_multicore
  tinyusb_device
  tinyusb_host
)
target_compile_definitions(pico_pio_usb PUBLIC
  PIO_USB_USE_TINYUSB=1
  CFG_TUH_RPI_PIO_USB=1
  CFG_TUH_ENABLED=1
  CFG_TUD_ENABLED=1
)

add_executable(${target_name})

target_include_directories(${target_name} PRIVATE
  ${CANETA_C_PATH}
  ${CANETA_RP2040_PATH}
  ${PIO_USB_PATH}/src
)

target_sources(${target_name} PRIVATE
  ${CANETA_RP2040_PATH}/main.c
  ${CANETA_RP2040_PATH}/tusb_config.h
)

# print memory usage, enable all warnings
target_link_options(${target_name} PRIVATE -Xlinker --print-memory-usage)
target_compile_options(${target_name} PRIVATE -Wall -Wextra)

# use tinyusb implementation
target_compile_definitions(${target_name} PRIVATE PIO_USB_USE_TINYUSB)

# needed so tinyusb can find tusb_config.h
target_include_directories(${target_name} PRIVATE ${CMAKE_CURRENT_LIST_DIR})

target_compile_definitions(${target_name} PRIVATE
  CFG_TUH_HID=2
  CFG_TUSB_DEBUG=1
)

target_link_libraries(${target_name} PRIVATE
  pico_stdlib
  pico_pio_usb
  tinyusb_host
  hardware_pio
  hardware_dma
  pico_multicore
  pico_unique_id
  hardware_uart
  tinyusb_host
)
target_link_libraries(${target_name} PRIVATE caneta-c)
pico_add_extra_outputs(${target_name})

pico_enable_stdio_uart(${target_name} 0)

# macOS Upload using picotool
add_custom_target(upload
  COMMAND picotool load -f ${OUTPUT_FILE}
  DEPENDS ${target_name}
  COMMENT "Uploading ${target_name}.uf2 to RP2040 via picotool"
)