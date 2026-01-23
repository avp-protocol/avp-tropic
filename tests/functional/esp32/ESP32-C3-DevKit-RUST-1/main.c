/**
 * @file main.c
 * @brief Common entrypoint for running functional tests against ESP32-C3-DevKit-RUST-1.
 * @copyright Copyright (c) 2020-2025 Tropic Square s.r.o.
 *
 * @license For the license see LICENSE.md in the root directory of this source tree.
 */

#include <stdbool.h>
#include <stdio.h>

#include "driver/gpio.h"
#include "esp_err.h"
#include "libtropic.h"
#include "libtropic_common.h"
#include "libtropic_functional_tests.h"
#include "libtropic_logging.h"
#include "libtropic_port_esp_idf.h"
#include "lt_test_common.h"

#if LT_USE_MBEDTLS_V4
#include "libtropic_mbedtls_v4.h"
#define CRYPTO_CTX_TYPE lt_ctx_mbedtls_v4_t
#endif

void app_main(void)
{
    // Declarations
    psa_status_t psa_init_status = PSA_ERROR_GENERIC_ERROR;  // The value needs to be different than PSA_SUCCESS to
                                                             // indicate that psa_crypto_init was not called.
    esp_err_t esp_ret = ESP_OK;
    lt_handle_t *__lt_handle__ = NULL;  // __lt_handle__ identifier is used by the test registry.
#if LT_SEPARATE_L3_BUFF
    uint8_t *l3_buffer = NULL;
#endif
    lt_dev_esp_idf_t *device = NULL;
    CRYPTO_CTX_TYPE *crypto_ctx = NULL;

// CFP initialization
#if LT_USE_MBEDTLS_V4
    psa_init_status = psa_crypto_init();
    if (psa_init_status != PSA_SUCCESS) {
        LT_LOG_ERROR("PSA Crypto initialization failed, status=%d (psa_status_t)", psa_init_status);
        goto app_main_cleanup;
    }
#endif

    // Handle initialization
    __lt_handle__ = (lt_handle_t *)malloc(sizeof(lt_handle_t));
    if (!__lt_handle__) {
        LT_LOG_ERROR("malloc() failed when allocating handle!");
        goto app_main_cleanup;
    }

#if LT_SEPARATE_L3_BUFF
    l3_buffer = (uint8_t *)malloc(LT_SIZE_OF_L3_BUFF);
    if (!l3_buffer) {
        LT_LOG_ERROR("malloc() failed when allocating L3 buffer!");
        goto app_main_cleanup;
    }
    __lt_handle__->l3.buff = l3_buffer;
    __lt_handle__->l3.buff_len = LT_SIZE_OF_L3_BUFF;
#endif

    // Device mappings
    // Initialize device before handing handle to the test.
    device = (lt_dev_esp_idf_t *)malloc(sizeof(lt_dev_esp_idf_t));
    if (!device) {
        LT_LOG_ERROR("malloc() failed when allocation device structure!");
        goto app_main_cleanup;
    }

    // Warning: using GPIO pins that conflict with ADC and I2C pins.
    device->spi_host_id = SPI2_HOST;
    device->spi_cs_gpio_pin = GPIO_NUM_8;  // I2C SCL
    device->spi_miso_pin = GPIO_NUM_0;     // ADC1_0
    device->spi_mosi_pin = GPIO_NUM_1;     // ADC1_1
    device->spi_clk_pin = GPIO_NUM_3;      // ADC1_3
    device->spi_clk_hz = 5000000;          // 5 MHz
#if LT_USE_INT_PIN
    device->int_gpio_pin = GPIO_NUM_10;  // I2C SDA
#endif
    // device->spi_handle = spi_handle;
    __lt_handle__->l2.device = device;

#if LT_USE_INT_PIN
    // This function has to be called only once.
    // The call is needed because the Libtropic ESP-IDF HAL uses GPIO interrupts.
    esp_ret = gpio_install_isr_service(0);
    if (esp_ret != ESP_OK) {
        LT_LOG_ERROR("gpio_install_isr_service() failed: %s", esp_err_to_name(esp_ret));
        goto app_main_cleanup;
    }
#endif

    // CAL context
    crypto_ctx = (CRYPTO_CTX_TYPE *)malloc(sizeof(CRYPTO_CTX_TYPE));
    if (!crypto_ctx) {
        LT_LOG_ERROR("malloc() failed when allocating crypto context!");
        goto app_main_cleanup;
    }
    __lt_handle__->l3.crypto_ctx = crypto_ctx;

    // Test code (correct test function is selected automatically per binary)
#include "lt_test_registry.c.inc"

    LT_FINISH_TEST();

app_main_cleanup:
#if LT_USE_MBEDTLS_V4
    if (psa_init_status == PSA_SUCCESS) {
        mbedtls_psa_crypto_free();
    }
#endif

    free(__lt_handle__);
#if LT_SEPARATE_L3_BUFF
    free(l3_buffer);
#endif
    free(device);
    free(crypto_ctx);

    // Delegate ESP errors handling to the ESP_ERROR_CHECK macro.
    if (esp_ret != ESP_OK) {
        ESP_ERROR_CHECK(esp_ret);
    }
}