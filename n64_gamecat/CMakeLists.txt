# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)
set(PICO_SDK_PATH "/Users/andrew/Development/pico-sdk")

set(PICO_BOARD solderparty_rp2350_stamp_xl CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

if (PICO_SDK_VERSION_STRING VERSION_LESS "1.4.0")
  message(FATAL_ERROR "Raspberry Pi Pico SDK version 1.4.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()

project(n64_gamecat C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

add_executable(n64_gamecat n64_gamecat.cpp
        Wire.cpp
        Wire.h
        gpio_cfg.h)

pico_set_program_name(n64_gamecat "n64_gamecat")
pico_set_program_version(n64_gamecat "0.1")

# no_flash means the target is to run from RAM
#pico_set_binary_type(n64_gamecat no_flash)

pico_enable_stdio_uart(n64_gamecat 1)
pico_enable_stdio_usb(n64_gamecat 0)

# Add the standard library to the build
target_link_libraries(n64_gamecat
        pico_stdlib)

# Add the standard include files to the build
target_include_directories(n64_gamecat PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}
  ${CMAKE_CURRENT_LIST_DIR}/.. # for our common lwipopts or any other standard includes, if required
)

# Add any user requested libraries
target_link_libraries(n64_gamecat 
        hardware_dma
        hardware_pio
        )

pico_add_extra_outputs(n64_gamecat)

pico_generate_pio_header(n64_gamecat ${CMAKE_CURRENT_LIST_DIR}/blink.pio)
pico_generate_pio_header(n64_gamecat ${CMAKE_CURRENT_LIST_DIR}/demo.pio)
pico_generate_pio_header(n64_gamecat ${CMAKE_CURRENT_LIST_DIR}/demo_read.pio)
