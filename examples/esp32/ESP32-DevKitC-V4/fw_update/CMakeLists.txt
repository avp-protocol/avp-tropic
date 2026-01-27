cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
idf_build_set_property(MINIMAL_BUILD ON)
project(fw_update)

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
