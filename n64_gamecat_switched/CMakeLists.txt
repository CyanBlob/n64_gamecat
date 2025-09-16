# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD solderparty_rp2350_stamp_xl CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(n64_gamecat_switched C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

add_executable(n64_gamecat_switched n64_gamecat_switched.cpp )

pico_set_program_name(n64_gamecat_switched "n64_gamecat_switched")
pico_set_program_version(n64_gamecat_switched "0.1")

# no_flash means the target is to run from RAM
# pico_set_binary_type(n64_gamecat_switched no_flash)
pico_set_binary_type(n64_gamecat_switched copy_to_ram)

# Generate PIO header
pico_generate_pio_header(n64_gamecat_switched ${CMAKE_CURRENT_LIST_DIR}/n64_capture.pio)
pico_generate_pio_header(n64_gamecat_switched ${CMAKE_CURRENT_LIST_DIR}/n64_respond_tokens.pio)

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(n64_gamecat_switched 1)
pico_enable_stdio_usb(n64_gamecat_switched 0)

# Add the standard library to the build
target_link_libraries(n64_gamecat_switched
        pico_stdlib)

# Add the standard include files to the build
target_include_directories(n64_gamecat_switched PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
)

# Add any user requested libraries
target_link_libraries(n64_gamecat_switched 
        hardware_spi
        hardware_i2c
        hardware_dma
        hardware_pio
        )

pico_add_extra_outputs(n64_gamecat_switched)
