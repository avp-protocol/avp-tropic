/**
 * @file main.c
 * @brief Utility for dumping certificates from TROPIC01 USB Devkit for Linux. Part of the Full chain verification
 * example.
 * @copyright Copyright (c) 2020-2026 Tropic Square s.r.o.
 *
 * @license For the license see file LICENSE.txt file in the root directory of this source tree.
 */

#include <stdio.h>
#include <string.h>
#include <time.h>

#include "libtropic.h"
#include "libtropic_common.h"
#include "libtropic_mbedtls_v4.h"
#include "libtropic_port_posix_usb_dongle.h"
#include "psa/crypto.h"

lt_ret_t dump_cert_store(lt_handle_t *lt_handle)
{
    uint8_t cert1[TR01_L2_GET_INFO_REQ_CERT_SIZE_SINGLE] = {0}, cert2[TR01_L2_GET_INFO_REQ_CERT_SIZE_SINGLE] = {0},
            cert3[TR01_L2_GET_INFO_REQ_CERT_SIZE_SINGLE] = {0}, cert4[TR01_L2_GET_INFO_REQ_CERT_SIZE_SINGLE] = {0};

    struct lt_cert_store_t store
        = {.certs = {cert1, cert2, cert3, cert4},
           .buf_len = {TR01_L2_GET_INFO_REQ_CERT_SIZE_SINGLE, TR01_L2_GET_INFO_REQ_CERT_SIZE_SINGLE,
                       TR01_L2_GET_INFO_REQ_CERT_SIZE_SINGLE, TR01_L2_GET_INFO_REQ_CERT_SIZE_SINGLE}};

    // Reading X509 Certificate Store
    printf("Reading certificates from TROPIC01...\n");
    lt_ret_t ret = lt_get_info_cert_store(lt_handle, &store);
    if (ret != LT_OK) {
        fprintf(stderr, "Failed to retrieve the certificates, ret=%s\n", lt_ret_verbose(ret));
        return LT_FAIL;
    }

    // Dump the certificates to files
    const char *names[LT_NUM_CERTIFICATES]
        = {"t01_ese_cert.der", "t01_xxxx_ca_cert.der", "t01_ca_cert.der", "tropicsquare_root_ca_cert.der"};

    printf("Writing certificates to files...\n");
    for (int i = 0; i < LT_NUM_CERTIFICATES; i++) {
        if (store.cert_len[i] == 0) {
            fprintf(stderr, "Error: Certificate %d is empty!\n", i);
            return LT_FAIL;
        }
        FILE *f = fopen(names[i], "wb");
        if (!f) {
            fprintf(stderr, "Error: Couldn't open file %s!\n", names[i]);
            return LT_FAIL;
        }
        if (fwrite(store.certs[i], 1, store.cert_len[i], f) != store.cert_len[i]) {
            fprintf(stderr, "Error: Failed to write certificate %d to file!\n", i);
            fclose(f);
            return LT_FAIL;
        }
        fclose(f);
    }

    return LT_OK;
}

int main(void)
{
    // Cosmetics: Disable buffering to keep output in order. You do not need to do this in your app if you don't care
    // about stdout/stderr output being shuffled or you use stdout only (or different output mechanism altogether).
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);

    printf("====================================================\n");
    printf("==== TROPIC01 Certificate Chain Dumping Utility ====\n");
    printf("====================================================\n");

    // Cryptographic function provider initialization.
    //
    // In production, this would typically be done only once,
    // usually at the start of the application or before
    // the first use of cryptographic functions but no later than
    // the first occurrence of any Libtropic function
    psa_status_t status = psa_crypto_init();
    if (status != PSA_SUCCESS) {
        fprintf(stderr, "PSA Crypto initialization failed, status=%d (psa_status_t)\n", status);
        return -1;
    }

    // Libtropic handle.
    //
    // It is declared here (on stack) for
    // simplicity. In production, you put it on heap if needed.
    lt_handle_t lt_handle = {0};

    // Device structure.
    //
    // Modify this according to your environment. Default values
    // are compatible with RPi and our RPi shield.
    lt_dev_posix_usb_dongle_t device = {0};

    // LT_USB_DEVKIT_PATH is defined in CMakeLists.txt. Pass -DLT_USB_DEVKIT_PATH=<path>
    // to cmake if you want to change it.
    int dev_path_len = snprintf(device.dev_path, sizeof(device.dev_path), "%s", LT_USB_DEVKIT_PATH);
    if (dev_path_len < 0 || (size_t)dev_path_len >= sizeof(device.dev_path)) {
        fprintf(stderr, "Error: LT_USB_DEVKIT_PATH is too long for device.dev_path buffer (limit is %zu bytes).\n",
                sizeof(device.dev_path));
        mbedtls_psa_crypto_free();
        return -1;
    }

    device.baud_rate = 115200;
    lt_handle.l2.device = &device;

    // Crypto abstraction layer (CAL) context.
    lt_ctx_mbedtls_v4_t crypto_ctx;
    lt_handle.l3.crypto_ctx = &crypto_ctx;

    printf("Initializing handle...");
    lt_ret_t ret = lt_init(&lt_handle);
    if (LT_OK != ret) {
        fprintf(stderr, "\nFailed to initialize handle, ret=%s\n", lt_ret_verbose(ret));
        mbedtls_psa_crypto_free();
        return -1;
    }
    printf("OK\n");

    printf("Dumping certificates...\n");
    if (LT_OK != dump_cert_store(&lt_handle)) {
        fprintf(stderr, "Error: Couldn't dump certificates!\n");
        lt_deinit(&lt_handle);
        mbedtls_psa_crypto_free();
        return -1;
    }
    printf("Certificates dumped successfully!\n");

    printf("Deinitializing handle...");
    ret = lt_deinit(&lt_handle);
    if (LT_OK != ret) {
        fprintf(stderr, "\nFailed to deinitialize handle, ret=%s\n", lt_ret_verbose(ret));
        mbedtls_psa_crypto_free();
        return -1;
    }
    printf("OK\n");

    // Cryptographic function provider deinitialization.
    //
    // In production, this would be done only once, typically
    // during termination of the application.
    mbedtls_psa_crypto_free();

    return 0;
}