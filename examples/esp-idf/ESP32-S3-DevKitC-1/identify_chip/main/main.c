/**
 * @file main.c
 * @brief Example of reading information about the TROPIC01 chip and its firmware using Libtropic and
 * ESP32-S3-DevKitC-1.
 * @author Tropic Square s.r.o.
 *
 * @license For the license see file LICENSE.txt file in the root directory of this source tree.
 */

#include <inttypes.h>
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

#define TAG "identify_chip"

// Some libtropic helper functions expect a printf-like function for logging.
// We provide a simple wrapper around ESP_LOGI to adapt it.
int my_esp_logi_wrapper(const char *format, ...)
{
    // Add the log timestamp and TAG prefix so idf.py colors it.
    esp_log_write(ESP_LOG_INFO, TAG, "I (%lu) %s: ", esp_log_timestamp(), TAG);

    // Handle the format string and its arguments.
    va_list args;
    va_start(args, format);
    esp_log_writev(ESP_LOG_INFO, TAG, format, args);
    va_end(args);

    return 0;
}

void app_main(void)
{
    ESP_LOGI(TAG, "==============================================");
    ESP_LOGI(TAG, "==== TROPIC01 Chip Identification Example ====");
    ESP_LOGI(TAG, "==============================================");

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
    // Using FSPI and its default pins from the official pinout diagram.
    lt_dev_esp_idf_t device = {0};
    device.spi_host_id = SPI2_HOST;
    device.spi_cs_gpio_pin = GPIO_NUM_10;
    device.spi_miso_pin = GPIO_NUM_13;
    device.spi_mosi_pin = GPIO_NUM_11;
    device.spi_clk_pin = GPIO_NUM_12;
    device.spi_clk_hz = 5000000;  // 5 MHz
#if LT_USE_INT_PIN
    device.int_gpio_pin = GPIO_NUM_46;
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

    ESP_LOGI(TAG, "Reading data from chip...");

    uint8_t fw_ver[4];
    ret = lt_get_info_riscv_fw_ver(&lt_handle, fw_ver);
    if (ret != LT_OK) {
        ESP_LOGE(TAG, "Failed to get RISC-V FW version, ret=%s", lt_ret_verbose(ret));
        lt_deinit(&lt_handle);
        mbedtls_psa_crypto_free();
        return;
    }
    ESP_LOGI(TAG, "  RISC-V FW version: %" PRIX8 ".%" PRIX8 ".%" PRIX8 " (.%" PRIX8 ")", fw_ver[3], fw_ver[2],
             fw_ver[1], fw_ver[0]);

    ret = lt_get_info_spect_fw_ver(&lt_handle, fw_ver);
    if (ret != LT_OK) {
        ESP_LOGE(TAG, "Failed to get SPECT FW version, ret=%s", lt_ret_verbose(ret));
        lt_deinit(&lt_handle);
        mbedtls_psa_crypto_free();
        return;
    }
    ESP_LOGI(TAG, "  SPECT FW version: %" PRIX8 ".%" PRIX8 ".%" PRIX8 " (.%" PRIX8 ")", fw_ver[3], fw_ver[2], fw_ver[1],
             fw_ver[0]);

    // We need to do the maintenance reboot to check bootloader version and FW bank headers in the Startup Mode.
    ESP_LOGI(TAG, "Sending maintenance reboot request...");
    ret = lt_reboot(&lt_handle, TR01_MAINTENANCE_REBOOT);
    if (ret != LT_OK) {
        ESP_LOGE(TAG, "Maintenance reboot failed, ret=%s", lt_ret_verbose(ret));
        lt_deinit(&lt_handle);
        mbedtls_psa_crypto_free();
        return;
    }
    ESP_LOGI(TAG, "OK");

    ESP_LOGI(TAG, "Reading data from chip...");

    // When TROPIC01 is in Start-up Mode, we can get RISC-V bootloader version the same way as we got RISC-V FW version.
    ret = lt_get_info_riscv_fw_ver(&lt_handle, fw_ver);
    if (ret != LT_OK) {
        ESP_LOGE(TAG, "Failed to get RISC-V bootloader version, ret=%s", lt_ret_verbose(ret));
        lt_deinit(&lt_handle);
        mbedtls_psa_crypto_free();
        return;
    }
    ESP_LOGI(TAG, "  RISC-V bootloader version: %" PRIX8 ".%" PRIX8 ".%" PRIX8 " (.%" PRIX8 ")",
             (uint8_t)(fw_ver[3] & 0x7f), fw_ver[2], fw_ver[1], fw_ver[0]);

    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "Firmware bank headers:");
    ret = lt_print_fw_header(&lt_handle, TR01_FW_BANK_FW1, my_esp_logi_wrapper);
    if (ret != LT_OK) {
        ESP_LOGE(TAG, "Failed to print TR01_FW_BANK_FW1 header, ret=%s", lt_ret_verbose(ret));
        lt_deinit(&lt_handle);
        mbedtls_psa_crypto_free();
        return;
    }
    ESP_LOGI(TAG, "");
    ret = lt_print_fw_header(&lt_handle, TR01_FW_BANK_FW2, my_esp_logi_wrapper);
    if (ret != LT_OK) {
        ESP_LOGE(TAG, "Failed to print TR01_FW_BANK_FW2 header, ret=%s", lt_ret_verbose(ret));
        lt_deinit(&lt_handle);
        mbedtls_psa_crypto_free();
        return;
    }
    ESP_LOGI(TAG, "");
    ret = lt_print_fw_header(&lt_handle, TR01_FW_BANK_SPECT1, my_esp_logi_wrapper);
    if (ret != LT_OK) {
        ESP_LOGE(TAG, "Failed to print TR01_FW_BANK_SPECT1 header, ret=%s", lt_ret_verbose(ret));
        lt_deinit(&lt_handle);
        mbedtls_psa_crypto_free();
        return;
    }
    ESP_LOGI(TAG, "");
    ret = lt_print_fw_header(&lt_handle, TR01_FW_BANK_SPECT2, my_esp_logi_wrapper);
    if (ret != LT_OK) {
        ESP_LOGE(TAG, "Failed to print TR01_FW_BANK_SPECT2 header, ret=%s", lt_ret_verbose(ret));
        lt_deinit(&lt_handle);
        mbedtls_psa_crypto_free();
        return;
    }

    struct lt_chip_id_t chip_id = {0};

    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "Chip ID data:");
    ret = lt_get_info_chip_id(&lt_handle, &chip_id);
    if (ret != LT_OK) {
        ESP_LOGE(TAG, "Failed to get chip ID, ret=%s", lt_ret_verbose(ret));
        lt_deinit(&lt_handle);
        mbedtls_psa_crypto_free();
        return;
    }

    ret = lt_print_chip_id(&chip_id, my_esp_logi_wrapper);
    if (ret != LT_OK) {
        ESP_LOGE(TAG, "Failed to print chip ID, ret=%s", lt_ret_verbose(ret));
        lt_deinit(&lt_handle);
        mbedtls_psa_crypto_free();
        return;
    }

    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "Sending reboot request...");
    ret = lt_reboot(&lt_handle, TR01_REBOOT);
    if (ret != LT_OK) {
        ESP_LOGE(TAG, "Reboot failed, ret=%s", lt_ret_verbose(ret));
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
