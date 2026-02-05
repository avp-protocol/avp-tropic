#ifndef USER_SETTINGS_H
#define USER_SETTINGS_H

#define WOLFCRYPT_ONLY           // Build only wolfCrypt library.
#define NO_OLD_RNGNAME           // Resolves collision between STM32 HAL 'RNG' and WolfSSL 'RNG'.
// #define USE_FAST_MATH

// We will provide custom implementation for seed generation.
extern int wolfcrypt_custom_seed_gen(unsigned char* output, unsigned int sz);
#define CUSTOM_RAND_GENERATE_SEED wolfcrypt_custom_seed_gen

#define WOLFSSL_SMALL_STACK   // Offload stack usage to heap where possible.
#define WOLFSSL_MALLOC_CHECK  // Optional: Safety check for malloc failures.

#define NO_FILESYSTEM        // Prevents filesystem errors on bare metal.
#undef WOLFSSL_SYS_CA_CERTS  // Force disable system CA certs (fixes dirent.h / filesystem errors).
#define NO_WRITEV            // IO vector write support usually missing.
#define NO_WRITE_TEMP_FILES
#define NO_DEV_RANDOM        // We use STM32's RNG, not /dev/random.
#define NO_MAIN_DRIVER

#define WOLFSSL_USER_IO  // Disable the default BSD socket callbacks.

#define SINGLE_THREADED  // No threads.

#endif /* USER_SETTINGS_H */