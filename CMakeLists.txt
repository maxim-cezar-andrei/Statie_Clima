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
set(picotoolVersion 2.2.0-a4)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico2_w CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(StatieClima C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Main file is StatieClima.c
add_executable(StatieClima StatieClima.c)

pico_set_program_name(StatieClima "StatieClima")
pico_set_program_version(StatieClima "0.1")

# Modificări pentru activarea ieșirii text prin USB (Consolă serială)
pico_enable_stdio_uart(StatieClima 0)
pico_enable_stdio_usb(StatieClima 1)

# --- CONFIGURARE LIBRĂRII HARDWARE PENTRU PROIECT ---
# Aici am unit biblioteca standard cu toate perifericele folosite de codul tău în C
target_link_libraries(StatieClima
        pico_stdlib
        hardware_gpio
        hardware_adc
        hardware_i2c
        hardware_pwm
)

# Add the standard include files to the build
target_include_directories(StatieClima PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
)

pico_add_extra_outputs(StatieClima)