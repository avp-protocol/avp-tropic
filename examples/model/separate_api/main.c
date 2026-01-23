/**
 * @file main.c
 * @brief Simple example of using Libtropic's Separate API with the model.
 * @author Tropic Square s.r.o.
 *
 * @license For the license see file LICENSE.txt file in the root directory of this source tree.
 */

#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "libtropic.h"
#include "libtropic_common.h"
#include "libtropic_l2.h"
#include "libtropic_l3.h"
#include "libtropic_mbedtls_v4.h"
#include "libtropic_port_posix_tcp.h"
#include "psa/crypto.h"

// @brief Message to send with Ping L3 command.
#define PING_MSG "This is Hello World message from TROPIC01!!"
// Size of the Ping message, including '\0'.
#define PING_MSG_SIZE 44

// Choose pairing keypair for slot 0.
#define LT_EX_SH0_PRIV sh0priv_prod0
#define LT_EX_SH0_PUB sh0pub_prod0

int main(void)
{
    // Cosmetics: Disable buffering to keep output in order. You do not need to do this in your app if you don't care
    // about stdout/stderr output being shuffled or you use stdout only (or different output mechanism altogether).
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);

    printf("========================================\n");
    printf("====  TROPIC01 Separate API Example ====\n");
    printf("========================================\n");

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

    // Initialize device before handing handle to the test.
    lt_dev_posix_tcp_t device;
    device.addr = inet_addr("127.0.0.1");
    device.port = 28992;
    lt_handle.l2.device = &device;

    // Generate seed for the PRNG and seed it.
    // Note: model uses rand(), which is not cryptographically secure. Better alternative should be used in production.
    unsigned int prng_seed;
    if (0 != getentropy(&prng_seed, sizeof(prng_seed))) {
        fprintf(stderr, "main: getentropy() failed (%s)!\n", strerror(errno));
        mbedtls_psa_crypto_free();
        return -1;
    }
    srand(prng_seed);
    printf("PRNG initialized with seed=%u\n", prng_seed);

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

    // We need to ensure we are not in the Startup Mode, as L3 commands are available only in the Application Firmware.
    printf("Sending reboot request...");
    ret = lt_reboot(&lt_handle, TR01_REBOOT);
    if (ret != LT_OK) {
        fprintf(stderr, "\nlt_reboot() failed, ret=%s\n", lt_ret_verbose(ret));
        lt_deinit(&lt_handle);
        mbedtls_psa_crypto_free();
        return -1;
    }
    printf("OK\n");

    printf("Getting Certificate Store from TROPIC01...");
    uint8_t cert1[TR01_L2_GET_INFO_REQ_CERT_SIZE_SINGLE], cert2[TR01_L2_GET_INFO_REQ_CERT_SIZE_SINGLE],
        cert3[TR01_L2_GET_INFO_REQ_CERT_SIZE_SINGLE], cert4[TR01_L2_GET_INFO_REQ_CERT_SIZE_SINGLE];
    struct lt_cert_store_t store
        = {.certs = {cert1, cert2, cert3, cert4},
           .buf_len = {TR01_L2_GET_INFO_REQ_CERT_SIZE_SINGLE, TR01_L2_GET_INFO_REQ_CERT_SIZE_SINGLE,
                       TR01_L2_GET_INFO_REQ_CERT_SIZE_SINGLE, TR01_L2_GET_INFO_REQ_CERT_SIZE_SINGLE}};
    ret = lt_get_info_cert_store(&lt_handle, &store);
    if (LT_OK != ret) {
        fprintf(stderr, "\nFailed to get Certificate Store, ret=%s\n", lt_ret_verbose(ret));
        lt_deinit(&lt_handle);
        mbedtls_psa_crypto_free();
        return -1;
    }
    printf("OK\n");

    // Get only stpub, we don't verify certificate chain here.
    printf("Getting stpub key from Certificate Store...");
    uint8_t stpub[TR01_STPUB_LEN];
    ret = lt_get_st_pub(&store, stpub);
    if (LT_OK != ret) {
        fprintf(stderr, "\nFailed to get stpub key, ret=%s\n", lt_ret_verbose(ret));
        lt_deinit(&lt_handle);
        mbedtls_psa_crypto_free();
        return -1;
    }
    printf("OK\n");

    //---------------------------------------------------------------------------------------//
    // Separated API calls for starting a secure session.
    lt_host_eph_keys_t host_eph_keys = {0};

    // Initialize session from a server side by creating host_eph_keys.ehpriv and host_eph_keys.ehpub,
    // L2 request is prepared into handle's buffer (lt_handle.l2_buff).
    printf("Executing lt_out__session_start()...");
    ret = lt_out__session_start(&lt_handle, TR01_PAIRING_KEY_SLOT_INDEX_0, &host_eph_keys);
    if (LT_OK != ret) {
        fprintf(stderr, "\nlt_out__session_start() failed, ret=%s\n", lt_ret_verbose(ret));
        lt_deinit(&lt_handle);
        mbedtls_psa_crypto_free();
        return -1;
    }
    printf("OK\n");

    // handle's buffer (lt_handle.l2_buff) now contains data which must be transferred over a tunnel to TROPIC01.

    // Following L2 functions are called on a remote host.
    printf("Executing lt_l2_send()...");
    ret = lt_l2_send(&lt_handle.l2);
    if (LT_OK != ret) {
        fprintf(stderr, "\nlt_l2_send() failed, ret=%s\n", lt_ret_verbose(ret));
        lt_deinit(&lt_handle);
        mbedtls_psa_crypto_free();
        return -1;
    }
    printf("OK\n");

    printf("Executing lt_l2_receive()...");
    ret = lt_l2_receive(&lt_handle.l2);
    if (LT_OK != ret) {
        fprintf(stderr, "\nlt_l2_receive() failed, ret=%s\n", lt_ret_verbose(ret));
        lt_deinit(&lt_handle);
        mbedtls_psa_crypto_free();
        return -1;
    }
    printf("OK\n");

    // Handle's buffer (lt_handle.l2_buff) now contains data which must be transferred over a tunnel back to the server.

    // Once data are back on server's side, bytes are copied into lt_handle.l2_buff.
    // Then, the following L2 function is called on the server side.
    // This function prepares AES-GCM contexts for the session.
    printf("Executing lt_in__session_start()...");
    ret = lt_in__session_start(&lt_handle, stpub, TR01_PAIRING_KEY_SLOT_INDEX_0, LT_EX_SH0_PRIV, LT_EX_SH0_PUB,
                               &host_eph_keys);
    if (LT_OK != ret) {
        fprintf(stderr, "\nlt_in__session_start failed, ret=%s\n", lt_ret_verbose(ret));
        lt_deinit(&lt_handle);
        mbedtls_psa_crypto_free();
        return -1;
    }
    printf("OK\n");

    // Now, we can use separate API for the Ping command to send a message to TROPIC01 and receive a response.
    uint8_t recv_buf[PING_MSG_SIZE];
    printf("Executing lt_out__ping()...");
    ret = lt_out__ping(&lt_handle, (const uint8_t *)PING_MSG, PING_MSG_SIZE);
    if (LT_OK != ret) {
        fprintf(stderr, "\nlt_out__ping() failed, ret=%s\n", lt_ret_verbose(ret));
        lt_session_abort(&lt_handle);
        lt_deinit(&lt_handle);
        mbedtls_psa_crypto_free();
        return -1;
    }
    printf("OK\n");

    printf("Executing lt_l2_send_encrypted_cmd()...");
    ret = lt_l2_send_encrypted_cmd(&lt_handle.l2, lt_handle.l3.buff, lt_handle.l3.buff_len);
    if (LT_OK != ret) {
        fprintf(stderr, "\nlt_l2_send_encrypted_cmd() failed, ret=%s\n", lt_ret_verbose(ret));
        lt_session_abort(&lt_handle);
        lt_deinit(&lt_handle);
        mbedtls_psa_crypto_free();
        return -1;
    }
    printf("OK\n");

    printf("Executing lt_l2_recv_encrypted_res()...");
    ret = lt_l2_recv_encrypted_res(&lt_handle.l2, lt_handle.l3.buff, lt_handle.l3.buff_len);
    if (LT_OK != ret) {
        fprintf(stderr, "\nlt_l2_recv_encrypted_res() failed, ret=%s\n", lt_ret_verbose(ret));
        lt_session_abort(&lt_handle);
        lt_deinit(&lt_handle);
        mbedtls_psa_crypto_free();
        return -1;
    }
    printf("OK\n");

    printf("Executing lt_in__ping()...");
    ret = lt_in__ping(&lt_handle, recv_buf, PING_MSG_SIZE);
    if (LT_OK != ret) {
        fprintf(stderr, "\nlt_in__ping() failed, ret=%s\n", lt_ret_verbose(ret));
        lt_session_abort(&lt_handle);
        lt_deinit(&lt_handle);
        mbedtls_psa_crypto_free();
        return -1;
    }
    printf("OK\n");

    printf("\t--> Message sent to TROPIC01: '%s'\n", PING_MSG);
    printf("\t<-- Message received from TROPIC01: '%s'\n", recv_buf);

    printf("Aborting Secure Session...");
    ret = lt_session_abort(&lt_handle);
    if (LT_OK != ret) {
        fprintf(stderr, "\nFailed to abort Secure Session, ret=%s\n", lt_ret_verbose(ret));
        lt_deinit(&lt_handle);
        mbedtls_psa_crypto_free();
        return -1;
    }
    printf("OK\n");

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