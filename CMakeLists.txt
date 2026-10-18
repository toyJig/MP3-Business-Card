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
set(sdkVersion 2.3.1)
set(toolchainVersion 15_2_Rel1)
set(picotoolVersion 2.3.1)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico CACHE STRING "Board type")
# set(PICO_FLASH_SIZE_BYTES 16777216 CACHE STRING "Flash size in bytes" FORCE)

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(Mp3-Prototype C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1
#yanks all files ending in cpp and .h in /src for build
file(GLOB SRC_FILES CONFIGURE_DEPENDS 
    "src/*.cpp" 
    "src/*.c" 
    "include/*.hpp"
    "include/*.h"
)

# Embed the MP3 as a generated C++ array so we get stable, predictable symbol names without
# path-dependent objcopy naming differences across platforms.
set(MP3_INPUT_FILE "${CMAKE_CURRENT_SOURCE_DIR}/mp3_src/audio.mp3")
set(MP3_C_FILE "${CMAKE_CURRENT_BINARY_DIR}/audio_blob.cpp")

file(READ "${MP3_INPUT_FILE}" MP3_HEX_DATA HEX)
string(REGEX REPLACE "([0-9a-fA-F][0-9a-fA-F])" "0x\\1, " MP3_BYTES "${MP3_HEX_DATA}")
string(REGEX REPLACE ", $" "" MP3_BYTES "${MP3_BYTES}")
file(WRITE "${MP3_C_FILE}" "#include <cstddef>\nextern \"C\" const unsigned char audio_mp3[] = {\n${MP3_BYTES}\n};\nextern \"C\" const size_t audio_mp3_size = sizeof(audio_mp3);\n")

# Create the executable using the generated blob source
add_executable(Mp3-Prototype ${SRC_FILES} ${MP3_C_FILE})
target_include_directories(Mp3-Prototype PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/src
    ${CMAKE_CURRENT_LIST_DIR}/include
)

#include helix mp3 decoder library folder
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/lib/picomp3lib/src)
#auto generate header for the DAC pio
pico_generate_pio_header(Mp3-Prototype ${CMAKE_CURRENT_LIST_DIR}/src/mcp4822.pio)


pico_set_program_name(Mp3-Prototype "Mp3-Prototype")
pico_set_program_version(Mp3-Prototype "0.1")

# Enable/disable output over UART/USB
pico_enable_stdio_usb(Mp3-Prototype 1)
pico_enable_stdio_uart(Mp3-Prototype 0)

# Add the standard library to the build
target_link_libraries(Mp3-Prototype
    pico_stdlib
    hardware_i2c
    hardware_spi
    hardware_dma
    hardware_pio
    hardware_clocks
    pico_multicore
    picomp3lib #helix decoder lib
)

pico_add_extra_outputs(Mp3-Prototype)


