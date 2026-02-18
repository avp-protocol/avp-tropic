# CMakeLists.txt for STM32U5 Tropic Click HAL
#
# This HAL targets:
# - STM32U5 series MCUs (STM32U575, STM32U585, etc.)
# - MikroE Secure Tropic Click board
# - AVP Hardware conformance

cmake_minimum_required(VERSION 3.16)

# HAL library
add_library(libtropic_hal_stm32u5_tropic_click STATIC
    libtropic_port_stm32u5_tropic_click.c
)

target_include_directories(libtropic_hal_stm32u5_tropic_click PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(libtropic_hal_stm32u5_tropic_click PUBLIC
    libtropic
)

# STM32U5 HAL dependency (user must provide)
# target_link_libraries(libtropic_hal_stm32u5_tropic_click PUBLIC stm32u5xx_hal)
