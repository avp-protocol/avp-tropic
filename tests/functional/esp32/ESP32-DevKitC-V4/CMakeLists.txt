cmake_minimum_required(VERSION 3.21.0)

# Set the target chip for this board.
set(TARGET "esp32")
# Set the toolchain file for ESP-IDF.
set(CMAKE_TOOLCHAIN_FILE $ENV{IDF_PATH}/tools/cmake/toolchain-${TARGET}.cmake)

# Project configuration for this board
project(libtropic_functional_tests_esp32_devkit_v4
	VERSION 0.1.0
	DESCRIPTION "Functional tests in ESP-IDF environment using ESP32-DevKitC-V4."
	LANGUAGES C)

include(${CMAKE_CURRENT_LIST_DIR}/../common.cmake)
