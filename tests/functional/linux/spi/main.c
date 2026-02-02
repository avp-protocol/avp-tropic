/**
 * @file main.c
 * @copyright Copyright (c) 2020-2026 Tropic Square s.r.o.
 *
 * @license For the license see file LICENSE.txt file in the root directory of this source tree.
 */

#include <string.h>
#include <time.h>

#include "libtropic.h"
#include "libtropic_common.h"
#include "libtropic_functional_tests.h"
#include "libtropic_logging.h"
#include "libtropic_port_linux_spi.h"

#if LT_USE_TREZOR_CRYPTO
#include "libtropic_trezor_crypto.h"
#define CRYPTO_CTX_TYPE lt_ctx_trezor_crypto_t
#elif LT_USE_MBEDTLS_V4
#include "libtropic_mbedtls_v4.h"
#include "psa/crypto.h"
#define CRYPTO_CTX_TYPE lt_ctx_mbedtls_v4_t
#elif LT_USE_OPENSSL
#include "libtropic_openssl.h"
#define CRYPTO_CTX_TYPE lt_ctx_openssl_t
#elif LT_USE_WOLFCRYPT
#include "libtropic_wolfcrypt.h"
#include "wolfssl/wolfcrypt/error-crypt.h"
#include "wolfssl/wolfcrypt/wc_port.h"
#define CRYPTO_CTX_TYPE lt_ctx_wolfcrypt_t
#endif

static int cleanup(void)
{
    int ret = 0;

#if LT_USE_MBEDTLS_V4
    mbedtls_psa_crypto_free();
#elif LT_USE_WOLFCRYPT
    ret = wolfCrypt_Cleanup();
    if (ret != 0) {
        LT_LOG_ERROR("WolfCrypt cleanup failed, ret=%d (%s)", ret, wc_GetErrorString(ret));
        return ret;
    }
#endif

    return ret;
}

int main(void)
{
    int ret = 0;

    // CFP initialization
#if LT_USE_MBEDTLS_V4
    psa_status_t status = psa_crypto_init();
    if (status != PSA_SUCCESS) {
        LT_LOG_ERROR("PSA Crypto initialization failed, status=%d (psa_status_t)", status);
        return -1;
    }
#elif LT_USE_WOLFCRYPT
    ret = wolfCrypt_Init();
    if (ret != 0) {
        LT_LOG_ERROR("WolfCrypt initialization failed, ret=%d (%s)", ret, wc_GetErrorString(ret));
        return ret;
    }
#endif

    // Handle initialization
    lt_handle_t lt_handle = {0};
#if LT_SEPARATE_L3_BUFF
    uint8_t l3_buffer[LT_SIZE_OF_L3_BUFF] __attribute__((aligned(16))) = {0};
    lt_handle.l3.buff = l3_buffer;
    lt_handle.l3.buff_len = sizeof(l3_buffer);
#endif

    // Device mappings
    lt_dev_linux_spi_t device = {0};

    // LT_GPIO_DEV_PATH is defined in CMakeLists.txt.
    int dev_path_len = snprintf(device.gpio_dev, sizeof(device.gpio_dev), "%s", LT_GPIO_DEV_PATH);
    if (dev_path_len < 0 || (size_t)dev_path_len >= sizeof(device.gpio_dev)) {
        LT_LOG_ERROR("Error: LT_GPIO_DEV_PATH is too long for device.gpio_dev buffer (limit is %zu bytes).",
                     sizeof(device.gpio_dev));
        LT_UNUSED(cleanup());  // Not caring about return val - we fail anyway.
        return -1;
    }

    // LT_SPI_DEV_PATH is defined in CMakeLists.txt.
    dev_path_len = snprintf(device.spi_dev, sizeof(device.spi_dev), "%s", LT_SPI_DEV_PATH);
    if (dev_path_len < 0 || (size_t)dev_path_len >= sizeof(device.spi_dev)) {
        LT_LOG_ERROR("Error: LT_SPI_DEV_PATH is too long for device.spi_dev buffer (limit is %zu bytes).",
                     sizeof(device.spi_dev));
        LT_UNUSED(cleanup());  // Not caring about return val - we fail anyway.
        return -1;
    }

    device.spi_speed = 5000000;  // 5 MHz (change if needed).
    device.gpio_cs_num = 25;     // GPIO 25 as on RPi shield.
#if LT_USE_INT_PIN
    device.gpio_int_num = 5;  // GPIO 5 as on RPi shield.
#endif
    lt_handle.l2.device = &device;

    // CAL context (selectable)
    CRYPTO_CTX_TYPE crypto_ctx;
    lt_handle.l3.crypto_ctx = &crypto_ctx;

    // Test code (correct test function is selected automatically per binary).
    // __lt_handle__ identifier is used by the test registry.
    lt_handle_t *__lt_handle__ = &lt_handle;
#include "lt_test_registry.c.inc"

    ret = cleanup();

    return ret;
}