/**
 * @file main.c
 * @brief Simple "Hello, World!" example of using Libtropic with the ESP32-DevKit-V4.
 * @author Tropic Square s.r.o.
 *
 * @license For the license see file LICENSE.txt file in the root directory of this source tree.
 */

#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "libtropic.h"
#include "libtropic_common.h"
#include "libtropic_mbedtls_v4.h"
#include "libtropic_port_esp_idf.h"
#include "psa/crypto.h"

#define TAG "hello_world"

// Message to send with Ping L3 command.
#define PING_MSG "This is Hello World message from TROPIC01!!"
// Size of the Ping message, including '\0'.
#define PING_MSG_SIZE 44

// Choose pairing keypair for slot 0.
#if LT_USE_SH0_ENG_SAMPLE
#define LT_EX_SH0_PRIV sh0priv_eng_sample
#define LT_EX_SH0_PUB sh0pub_eng_sample
#elif LT_USE_SH0_PROD0
#define LT_EX_SH0_PRIV sh0priv_prod0
#define LT_EX_SH0_PUB sh0pub_prod0
#endif

void app_main(void)
{
    ESP_LOGI(TAG, "======================================");
    ESP_LOGI(TAG, "==== TROPIC01 Hello World Example ====");
    ESP_LOGI(TAG, "======================================");

    // Cryptographic function provider initialization.
    //
    // In production, this would typically be done only once,
    // usually at the start of the application or before
    // the first use of cryptographic functions but no later than
    // the first occurrence of any Libtropic function
    psa_status_t status = psa_crypto_init();
    if (status != PSA_SUCCESS) {
        ESP_LOGE(TAG, "PSA Crypto initialization failed, status=%d (psa_status_t)", status);
        return;
    }

    // Libtropic handle.
    //
    // It is declared here (on stack) for
    // simplicity. In production, you put it on heap if needed.
    lt_handle_t lt_handle = {0};

    // Device structure.
    //
    // Used default pins for a typical ESP32 DevKit; adjust if you use different wiring.
    lt_dev_esp_idf_t device = {0};
    device.spi_host_id = VSPI_HOST;
    device.spi_cs_gpio_pin = GPIO_NUM_5;
    device.spi_miso_pin = GPIO_NUM_19;
    device.spi_mosi_pin = GPIO_NUM_23;
    device.spi_clk_pin = GPIO_NUM_18;
    device.spi_clk_hz = 5000000;  // 5 MHz
#if LT_USE_INT_PIN
    device.int_gpio_pin = GPIO_NUM_4;
#endif
    lt_handle.l2.device = &device;

#if LT_USE_INT_PIN
    // This function has to be called only once.
    // The call is needed because the Libtropic ESP-IDF HAL uses GPIO interrupts.
    esp_err_t esp_ret = gpio_install_isr_service(0);
    if (esp_ret != ESP_OK) {
        ESP_LOGE(TAG, "gpio_install_isr_service() failed: %s", esp_err_to_name(esp_ret));
        lt_deinit(&lt_handle);
        mbedtls_psa_crypto_free();
    }
#endif

    // Crypto abstraction layer (CAL) context.
    lt_ctx_mbedtls_v4_t crypto_ctx;
    lt_handle.l3.crypto_ctx = &crypto_ctx;

    ESP_LOGI(TAG, "Initializing handle...");
    lt_ret_t ret = lt_init(&lt_handle);
    if (LT_OK != ret) {
        ESP_LOGE(TAG, "Failed to initialize handle, ret=%s", lt_ret_verbose(ret));
        lt_deinit(&lt_handle);
        mbedtls_psa_crypto_free();
        return;
    }
    ESP_LOGI(TAG, "OK");

    // First, we check versions of both updateable firmwares. To do that, we need TROPIC01 to **not** be in the Start-up
    // Mode. If there are valid firmwares, TROPIC01 will begin to execute them automatically on boot.
    ESP_LOGI(TAG, "Sending reboot request...");
    ret = lt_reboot(&lt_handle, TR01_REBOOT);
    if (ret != LT_OK) {
        ESP_LOGE(TAG, "Reboot failed, ret=%s", lt_ret_verbose(ret));
        lt_deinit(&lt_handle);
        mbedtls_psa_crypto_free();
        return;
    }
    ESP_LOGI(TAG, "OK");

    ESP_LOGI(TAG, "Starting Secure Session with key slot %d...", (int)TR01_PAIRING_KEY_SLOT_INDEX_0);
    ret = lt_verify_chip_and_start_secure_session(&lt_handle, LT_EX_SH0_PRIV, LT_EX_SH0_PUB,
                                                  TR01_PAIRING_KEY_SLOT_INDEX_0);
    if (LT_OK != ret) {
        ESP_LOGE(TAG, "Failed to start Secure Session with key %d, ret=%s", (int)TR01_PAIRING_KEY_SLOT_INDEX_0,
                 lt_ret_verbose(ret));
        ESP_LOGE(TAG,
                 "Check if you use correct SH0 keys! Hint: if you use an engineering sample chip, compile with "
                 "-DLT_SH0_KEYS=eng_sample");
        lt_deinit(&lt_handle);
        mbedtls_psa_crypto_free();
        return;
    }
    ESP_LOGI(TAG, "OK");

    uint8_t recv_buf[PING_MSG_SIZE];
    ESP_LOGI(TAG, "Sending Ping command...");
    ESP_LOGI(TAG, "  --> Message sent to TROPIC01: '%s'", PING_MSG);
    ret = lt_ping(&lt_handle, (const uint8_t *)PING_MSG, recv_buf, PING_MSG_SIZE);
    if (LT_OK != ret) {
        ESP_LOGE(TAG, "Ping command failed, ret=%s", lt_ret_verbose(ret));
        lt_session_abort(&lt_handle);
        lt_deinit(&lt_handle);
        mbedtls_psa_crypto_free();
        return;
    }
    ESP_LOGI(TAG, "  <-- Message received from TROPIC01: '%s'", recv_buf);

    ESP_LOGI(TAG, "Aborting Secure Session...");
    ret = lt_session_abort(&lt_handle);
    if (LT_OK != ret) {
        ESP_LOGE(TAG, "Failed to abort Secure Session, ret=%s", lt_ret_verbose(ret));
        lt_deinit(&lt_handle);
        mbedtls_psa_crypto_free();
        return;
    }
    ESP_LOGI(TAG, "OK");

    ESP_LOGI(TAG, "Deinitializing handle...");
    ret = lt_deinit(&lt_handle);
    if (LT_OK != ret) {
        ESP_LOGE(TAG, "Failed to deinitialize handle, ret=%s", lt_ret_verbose(ret));
        mbedtls_psa_crypto_free();
        return;
    }
    ESP_LOGI(TAG, "OK");

    // Cryptographic function provider deinitialization.
    //
    // In production, this would be done only once, typically
    // during termination of the application.
    mbedtls_psa_crypto_free();
}
