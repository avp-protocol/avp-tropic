/**
 * @file lt_openssl_hmac_sha256.c
 * @copyright Copyright (c) 2020-2025 Tropic Square s.r.o.
 *
 * @license For the license see LICENSE.md in the root directory of this source tree.
 */

#include <inttypes.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <stdint.h>

#include "libtropic_common.h"
#include "libtropic_logging.h"
#include "lt_hmac_sha256.h"

lt_ret_t lt_hmac_sha256(const uint8_t *key, const uint32_t key_len, const uint8_t *input, const uint32_t input_len,
                        uint8_t *output)
{
    EVP_MD_CTX *ctx = NULL;
    EVP_PKEY *pkey = NULL;
    unsigned long err_code;
    lt_ret_t ret = LT_OK;

    // Convert `key` raw bytes into an OpenSSL Key object.
    pkey = EVP_PKEY_new_raw_private_key(EVP_PKEY_HMAC, NULL, key, key_len);
    if (!pkey) {
        err_code = ERR_get_error();
        LT_LOG_ERROR("Failed to create HMAC-SHA256 key object, err_code=%lu (%s)", err_code,
                     ERR_error_string(err_code, NULL));
        ret = LT_CRYPTO_ERR;
        goto lt_hmac_sha256_cleanup;
    }

    // Create and initialize the HMAC-SHA256 context.
    ctx = EVP_MD_CTX_new();
    if (!ctx) {
        err_code = ERR_get_error();
        LT_LOG_ERROR("Failed to create HMAC-SHA256 context, err_code=%lu (%s)", err_code,
                     ERR_error_string(err_code, NULL));
        ret = LT_CRYPTO_ERR;
        goto lt_hmac_sha256_cleanup;
    }

    // Initialize the HMAC-SHA256 operation.
    if (!EVP_DigestSignInit(ctx, NULL, EVP_sha256(), NULL, pkey)) {
        err_code = ERR_get_error();
        LT_LOG_ERROR("Failed to initialize HMAC-SHA256 context, err_code=%lu (%s)", err_code,
                     ERR_error_string(err_code, NULL));
        ret = LT_CRYPTO_ERR;
        goto lt_hmac_sha256_cleanup;
    }

    // Provide the input data to be hashed.
    if (!EVP_DigestSignUpdate(ctx, input, input_len)) {
        err_code = ERR_get_error();
        LT_LOG_ERROR("Failed to update HMAC-SHA256 hash, err_code=%lu (%s)", err_code,
                     ERR_error_string(err_code, NULL));
        ret = LT_CRYPTO_ERR;
        goto lt_hmac_sha256_cleanup;
    }

    // Finalize the HMAC-SHA256 computation.
    size_t out_len = LT_HMAC_SHA256_HASH_LEN;
    if (!EVP_DigestSignFinal(ctx, output, &out_len)) {
        err_code = ERR_get_error();
        LT_LOG_ERROR("Failed to finalize HMAC-SHA256 hash, err_code=%lu (%s)", err_code,
                     ERR_error_string(err_code, NULL));
        ret = LT_CRYPTO_ERR;
        goto lt_hmac_sha256_cleanup;
    }

    if (out_len != LT_HMAC_SHA256_HASH_LEN) {
        LT_LOG_ERROR("HMAC-SHA256 output length mismatch! Current: %zu bytes, expected: %d bytes", out_len,
                     LT_HMAC_SHA256_HASH_LEN);
        ret = LT_CRYPTO_ERR;
        goto lt_hmac_sha256_cleanup;
    }

lt_hmac_sha256_cleanup:
    EVP_PKEY_free(pkey);
    EVP_MD_CTX_free(ctx);
    return ret;
}
