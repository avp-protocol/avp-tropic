cmake_minimum_required(VERSION 3.21.0)
include (FetchContent)

###########################################################################
#                                                                         #
#   Set up projects and paths                                             #
#                                                                         #
###########################################################################
project(libtropic_hw_wallet
        DESCRIPTION "Libtropic hardware wallet example on model."
        LANGUAGES C)

set(PATH_LIBTROPIC ../../../)

###########################################################################
#                                                                         #
#   Configuration                                                         #
#                                                                         #
###########################################################################
if(NOT UNIX)
    message(FATAL_ERROR "Model is currently compatible with UNIX-like systems only.")
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

# Customize libtropic's compilation
target_compile_options(tropic PRIVATE -ffunction-sections -fdata-sections)

# ------------------------------------------------------------------------
# External dependencies
# ------------------------------------------------------------------------

# MbedTLS v4.0.0
set(ENABLE_TESTING OFF CACHE BOOL "Disable mbedtls_v4 test building.")
set(ENABLE_PROGRAMS OFF CACHE BOOL "Disable mbedtls_v4 examples building.")
FetchContent_Declare(
    mbedtls_v4
    URL https://github.com/Mbed-TLS/mbedtls/releases/download/mbedtls-4.0.0/mbedtls-4.0.0.tar.bz2
    URL_HASH SHA256=2f3a47f7b3a541ddef450e4867eeecb7ce2ef7776093f3a11d6d43ead6bf2827
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(mbedtls_v4)
target_link_libraries(tropic PUBLIC mbedtls)


# Download ed25519 repository used for EdDSA signature verification
FetchContent_Declare(
    ed25519
    URL https://github.com/orlp/ed25519/archive/b1f19fab4aebe607805620d25a5e42566ce46a0e.zip
    URL_HASH SHA256=75f39e64f22ec7474e7881315a3f9135afe6c990737388fb16a6950911b55721
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(ed25519)

add_library(ed25519_lib STATIC
    ${ed25519_SOURCE_DIR}/src/verify.c
    ${ed25519_SOURCE_DIR}/src/add_scalar.c
    ${ed25519_SOURCE_DIR}/src/fe.c
    ${ed25519_SOURCE_DIR}/src/ge.c
    ${ed25519_SOURCE_DIR}/src/key_exchange.c
    ${ed25519_SOURCE_DIR}/src/keypair.c
    ${ed25519_SOURCE_DIR}/src/sc.c
    ${ed25519_SOURCE_DIR}/src/seed.c
    ${ed25519_SOURCE_DIR}/src/sha512.c
    ${ed25519_SOURCE_DIR}/src/sign.c
)

target_include_directories(ed25519_lib PUBLIC
    ${ed25519_SOURCE_DIR}/src/
)

###########################################################################
#                                                                         #
#   Set up sources and compilation                                        #
#                                                                         #
###########################################################################
# Add MbedTLS v4 CAL
add_subdirectory("${PATH_LIBTROPIC}/cal/mbedtls_v4" "mbedtls_v4_cal")
target_sources(tropic PRIVATE ${LT_CAL_SRCS})
target_include_directories(tropic PUBLIC ${LT_CAL_INC_DIRS})

# Add POSIX TCP HAL
add_subdirectory("${PATH_LIBTROPIC}/hal/posix/tcp" "posix_tcp_hal")
target_sources(tropic PRIVATE ${LT_HAL_SRCS})
target_include_directories(tropic PUBLIC ${LT_HAL_INC_DIRS})

# Add sources of this example
set(SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/main.c
)

# Define executable, pass defines, and link dependencies.
add_executable(${CMAKE_PROJECT_NAME} ${SOURCES})
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE tropic)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE ed25519_lib)