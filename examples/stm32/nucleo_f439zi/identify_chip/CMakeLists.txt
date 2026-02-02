cmake_minimum_required(VERSION 3.21.0)
include (FetchContent)

###########################################################################
#                                                                         #
#   Set up project and paths                                              #
#                                                                         #
###########################################################################

# Specify stm32 device
set(STM32_DEVICE "STM32F439xx")
# Openocd config file for used nucleo board
set(OPENOCD_CFG "/usr/share/openocd/scripts/board/stm32f429discovery.cfg")
# Path to linker script file
set(LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/STM32F429ZITX_FLASH.ld)
# Path to toolchain file
set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/toolchain.cmake)
# Paths to STM32 drivers.
# We provide a stripped-down snapshot of STM32CubeF4 repo in this example.
set(PATH_TO_STM32CUBEF4 ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/STM32CubeF4/Drivers )

# Path to Libtropic repository root
if(NOT DEFINED PATH_LIBTROPIC)
    set(PATH_LIBTROPIC "../../../../")
endif()

project(
    libtropic_identify_chip
    DESCRIPTION "Libtropic chip identification example on STM32 Nucleo F439ZI board"
    LANGUAGES C ASM)

###########################################################################
#                                                                         #
#   Configuration                                                         #
#                                                                         #
###########################################################################

# Serial number of the STLink device which will be used for flashing.
# You need to define this only if you have multiple STM32s connected using built-in STLink or 
# problems with OpenOCD's autodetection.
if (NOT DEFINED STLINK_SERIAL_NUMBER)
    set(STLINK_SERIAL_NUMBER "")
endif()

# Select pairing keys written during manufacturing into your TROPIC01
set(LT_SH0_KEYS "prod0" CACHE STRING "Choose which pairing keys in slot 0 will be used in this example")
set_property(CACHE LT_SH0_KEYS PROPERTY STRINGS "eng_sample" "prod0")

# These are internal macros for selecting SH0 keys
set(LT_USE_SH0_ENG_SAMPLE 0 CACHE INTERNAL "")
set(LT_USE_SH0_PROD0      0 CACHE INTERNAL "")

# Define SH0 macros based on selected string
if(LT_SH0_KEYS STREQUAL "eng_sample")
    message(STATUS "Using Engineering sample keys")
    set(LT_USE_SH0_ENG_SAMPLE 1)
    set(LT_USE_SH0_PROD0      0)
elseif(LT_SH0_KEYS STREQUAL "prod0")
    message(STATUS "Using Production 0 keys")
    set(LT_USE_SH0_ENG_SAMPLE 0)
    set(LT_USE_SH0_PROD0      1)
else()
    get_property(lt_sh0_keys_choices CACHE LT_SH0_KEYS PROPERTY STRINGS)
    message(FATAL_ERROR "Incorrect SH0 keys specified: '${LT_SH0_KEYS}'\nAvailable SH0 keys: ${lt_sh0_keys_choices}")
endif()

###########################################################################
#                                                                         #
#   Set up dependencies                                                   #
#                                                                         #
###########################################################################

# ------------------------------------------------------------------------
# Libtropic 
# ------------------------------------------------------------------------
# Add path to Libtropic source
add_subdirectory(${PATH_LIBTROPIC} "libtropic")

# ------------------------------------------------------------------------
# MbedTLS v4.0.0
# ------------------------------------------------------------------------

# Download MbedTLS
FetchContent_Declare(
    mbedtls_v4
    URL https://github.com/Mbed-TLS/mbedtls/releases/download/mbedtls-4.0.0/mbedtls-4.0.0.tar.bz2
    URL_HASH SHA256=2f3a47f7b3a541ddef450e4867eeecb7ce2ef7776093f3a11d6d43ead6bf2827
)

# We configure MbedTLS using config file with following configuration:
# - config.py preset "crypto_baremetal"
# - following options enabled:
#   MBEDTLS_PLATFORM_MS_TIME_ALT
#   MBEDTLS_HAVE_TIME
#   MBEDTLS_PSA_DRIVER_GET_ENTROPY
# - following options disabled:
#   MBEDTLS_PSA_BUILTIN_GET_ENTROPY
#   MBEDTLS_TEST_HOOKS
#   MBEDTLS_PSA_CRYPTO_BUILTIN_KEYS
# We need to set both CMake variables and compile definitions for MbedTLS to pick up our config file.
set(MBEDTLS_CONFIG_FILE "${CMAKE_CURRENT_SOURCE_DIR}/Src/mbedtls_v4/mbedtls_config.h")
set(TF_PSA_CRYPTO_CONFIG_FILE "${CMAKE_CURRENT_SOURCE_DIR}/Src/mbedtls_v4/crypto_config.h")
add_compile_definitions(MBEDTLS_CONFIG_FILE="${MBEDTLS_CONFIG_FILE}")
add_compile_definitions(TF_PSA_CRYPTO_CONFIG_FILE="${TF_PSA_CRYPTO_CONFIG_FILE}")
set(ENABLE_TESTING OFF CACHE BOOL "Disable mbedtls_v4 test building.")
set(ENABLE_PROGRAMS OFF CACHE BOOL "Disable mbedtls_v4 examples building.")
FetchContent_MakeAvailable(mbedtls_v4)
target_link_libraries(tropic PUBLIC mbedtls)

###########################################################################
#                                                                         #
#   Sources                                                               #
#                                                                         #
###########################################################################

# Add MbedTLS v4 CAL
add_subdirectory("${PATH_LIBTROPIC}cal/mbedtls_v4" "mbedtls_v4_cal")
target_sources(tropic PRIVATE ${LT_CAL_SRCS})
target_include_directories(tropic PUBLIC ${LT_CAL_INC_DIRS})

# Add Libtropic STM32 HAL
add_subdirectory("${PATH_LIBTROPIC}hal/stm32/nucleo_f439zi" "nucleo_f439zi_hal")

# Specify other source files
set(SOURCES
    # STM32 vendor HAL, BSP
    ${PATH_TO_STM32CUBEF4}/BSP/STM32F4xx_Nucleo_144/stm32f4xx_nucleo_144.c
    ${PATH_TO_STM32CUBEF4}/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal.c
    ${PATH_TO_STM32CUBEF4}/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_uart.c
    ${PATH_TO_STM32CUBEF4}/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rcc.c
    ${PATH_TO_STM32CUBEF4}/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rng.c
    ${PATH_TO_STM32CUBEF4}/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pwr_ex.c
    ${PATH_TO_STM32CUBEF4}/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_gpio.c
    ${PATH_TO_STM32CUBEF4}/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_cortex.c
    ${PATH_TO_STM32CUBEF4}/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_spi.c

    # Project sources
    ${CMAKE_CURRENT_SOURCE_DIR}/Src/main.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Src/stm32f4xx_it.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Src/stm32f4xx_hal_msp.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Src/system_stm32f4xx.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Src/startup_stm32f429zitx.s
    ${CMAKE_CURRENT_SOURCE_DIR}/Src/syscalls.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Src/sysmem.c

    # MbedTLS platform-specific implementations
    ${CMAKE_CURRENT_SOURCE_DIR}/Src/mbedtls_v4/mbedtls_platform.c

    # STM32 HAL
    ${LT_HAL_SRCS}
)

# Include path for directories containing header files
set(INCLUDE_DIRS
    # STM32 vendor HAL, BSP
    ${PATH_TO_STM32CUBEF4}/CMSIS/Core/Include/
    ${PATH_TO_STM32CUBEF4}/CMSIS/Device/ST/STM32F4xx/Include/
    ${PATH_TO_STM32CUBEF4}/BSP/STM32F4xx_Nucleo_144/
    ${PATH_TO_STM32CUBEF4}/STM32F4xx_HAL_Driver/Inc/

    # Project includes
    ${CMAKE_CURRENT_SOURCE_DIR}/Inc/

    # STM32 HAL
    ${LT_HAL_INC_DIRS}
)

# Define executable and configure (pass defines, includes and link dependencies).
set(EXE_NAME ${CMAKE_PROJECT_NAME}.elf)
add_executable(${EXE_NAME} ${SOURCES})
target_compile_definitions(${EXE_NAME} PRIVATE LT_USE_SH0_ENG_SAMPLE=${LT_USE_SH0_ENG_SAMPLE})
target_compile_definitions(${EXE_NAME} PRIVATE LT_USE_SH0_PROD0=${LT_USE_SH0_PROD0})
target_compile_definitions(${EXE_NAME} PRIVATE -D${STM32_DEVICE}) # Configure target MCU (used by STM32 HAL)
target_include_directories(${EXE_NAME} PRIVATE ${INCLUDE_DIRS})
target_link_libraries(${EXE_NAME} PRIVATE tropic)

###########################################################################
#                                                                         #
#   Helpers                                                               #
#                                                                         #
###########################################################################
# Define "flash" target for convenient flashing.
add_custom_target(flash
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/flash.sh ${CMAKE_CURRENT_BINARY_DIR}/${EXE_NAME} ${STLINK_SERIAL_NUMBER}
    DEPENDS ${EXE_NAME}
)