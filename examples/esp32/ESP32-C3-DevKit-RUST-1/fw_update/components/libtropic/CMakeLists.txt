cmake_minimum_required(VERSION 3.16)

# Path from this file to the libtropic repo root.
set(PATH_LIBTROPIC ${CMAKE_CURRENT_LIST_DIR}/../../../../../../)

# Register the libtropic component and list its dependencies.
idf_component_register(REQUIRES mbedtls esp_driver_spi esp_driver_gpio)

# Link the component library to the 'tropic' target from libtropic.
target_link_libraries(${COMPONENT_LIB} INTERFACE tropic)

