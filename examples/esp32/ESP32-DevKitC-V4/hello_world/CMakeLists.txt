cmake_minimum_required(VERSION 3.16)

# Select pairing keys written during manufacturing into your TROPIC01.
# Note for users: if you are writing your own application, the following logic
# with LT_SH0_KEYS is not necessary - use specific key pair, e.g. your own or 
# the ones from libtropic_common.h.
set(LT_SH0_KEYS "prod0" CACHE STRING "Choose which pairing keys in slot 0 will be used in this example")
set_property(CACHE LT_SH0_KEYS PROPERTY STRINGS "eng_sample" "prod0")

# These are internal macros for selecting SH0 keys.
set(LT_USE_SH0_ENG_SAMPLE 0 CACHE INTERNAL "")
set(LT_USE_SH0_PROD0      0 CACHE INTERNAL "")

if(LT_SH0_KEYS STREQUAL "eng_sample")
	set(LT_USE_SH0_ENG_SAMPLE 1)
	set(LT_USE_SH0_PROD0      0)
elseif(LT_SH0_KEYS STREQUAL "prod0")
	set(LT_USE_SH0_ENG_SAMPLE 0)
	set(LT_USE_SH0_PROD0      1)
else()
	get_property(lt_sh0_keys_choices CACHE LT_SH0_KEYS PROPERTY STRINGS)
	message(FATAL_ERROR "Incorrect SH0 keys specified: '${LT_SH0_KEYS}'\nAvailable SH0 keys: ${lt_sh0_keys_choices}")
endif()

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
idf_build_set_property(MINIMAL_BUILD ON)
project(hello_world)

# Add libtropic top-level project so its 'tropic' target is available to components.
# Add it after including IDF's project.cmake so IDF toolchain variables are configured.
set(PATH_LIBTROPIC ${CMAKE_CURRENT_LIST_DIR}/../../../../)
add_subdirectory(${PATH_LIBTROPIC} ${CMAKE_CURRENT_BINARY_DIR}/libtropic_build)

# Add libtropic's MbedTLS CAL.
add_subdirectory("${PATH_LIBTROPIC}cal/mbedtls_v4" ${CMAKE_CURRENT_BINARY_DIR}/mbedtls_v4_cal)
target_sources(tropic PRIVATE ${LT_CAL_SRCS})
target_include_directories(tropic PUBLIC ${LT_CAL_INC_DIRS})

# Add libtropic's ESP-IDF HAL.
add_subdirectory("${PATH_LIBTROPIC}hal/esp-idf" ${CMAKE_CURRENT_BINARY_DIR}/esp_idf_hal)
target_sources(tropic PRIVATE ${LT_HAL_SRCS})
target_include_directories(tropic PUBLIC ${LT_HAL_INC_DIRS})

# Link IDF components to libtropic's HAL and CAL, because they depend on them.
target_link_libraries(tropic PUBLIC
	idf::mbedtls
	idf::esp_driver_spi
	idf::esp_driver_gpio
)
