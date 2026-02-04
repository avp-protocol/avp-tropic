/**
 * @file lt_openssl_aesgcm.c
 * @copyright Copyright (c) 2020-2026 Tropic Square s.r.o.
 *
 * @license For the license see LICENSE.md in the root directory of this source tree.
 */

#include <inttypes.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "libtropic_common.h"
#include "libtropic_logging.h"
#include "libtropic_openssl.h"
#include "lt_aesgcm.h"

lt_ret_t lt_aesgcm_encrypt_init(void *ctx, const uint8_t *key, const uint32_t key_len)
{
    if (key_len != TR01_AES256_KEY_LEN) {
        LT_LOG_ERROR("Invalid AES-GCM key length: got %" PRIu32 " bytes, expected %d bytes", key_len,
                     TR01_AES256_KEY_LEN);
        return LT_PARAM_ERR;
    }

    lt_ctx_openssl_t *_ctx = (lt_ctx_openssl_t *)ctx;
    unsigned long err_code;

    // Initialize AES-GCM encryption context.
    _ctx->aesgcm_encrypt_ctx = EVP_CIPHER_CTX_new();
    if (!_ctx->aesgcm_encrypt_ctx) {
        err_code = ERR_get_error();
        LT_LOG_ERROR("Failed to allocate AES-GCM encryption context, err_code=%lu (%s)", err_code,
                     ERR_error_string(err_code, NULL));
        return LT_CRYPTO_ERR;
    }

    // Set cipher type.
    if (!EVP_EncryptInit_ex(_ctx->aesgcm_encrypt_ctx, EVP_aes_256_gcm(), NULL, NULL, NULL)) {
        err_code = ERR_get_error();
        LT_LOG_ERROR(
            "Failed to initialize AES-GCM encryption context with cipher type, err_code=%lu (%s)",
            err_code, ERR_error_string(err_code, NULL));
        return LT_CRYPTO_ERR;
    }

    // Set IV length.
    if (!EVP_CIPHER_CTX_ctrl(_ctx->aesgcm_encrypt_ctx, EVP_CTRL_GCM_SET_IVLEN, TR01_L3_IV_SIZE,
                             NULL)) {
        err_code = ERR_get_error();
        LT_LOG_ERROR(
            "Failed to initialize AES-GCM encryption context with IV length, err_code=%lu (%s)",
            err_code, ERR_error_string(err_code, NULL));
        return LT_CRYPTO_ERR;
    }

    // Set encryption key.
    if (!EVP_EncryptInit_ex(_ctx->aesgcm_encrypt_ctx, NULL, NULL, key, NULL)) {
        err_code = ERR_get_error();
        LT_LOG_ERROR("Failed to initialize AES-GCM encryption context with key, err_code=%lu (%s)",
                     err_code, ERR_error_string(err_code, NULL));
        return LT_CRYPTO_ERR;
    }

    return LT_OK;
}

lt_ret_t lt_aesgcm_decrypt_init(void *ctx, const uint8_t *key, const uint32_t key_len)
{
    if (key_len != TR01_AES256_KEY_LEN) {
        LT_LOG_ERROR("Invalid AES-GCM key length: got %" PRIu32 " bytes, expected %d bytes", key_len,
                     TR01_AES256_KEY_LEN);
        return LT_PARAM_ERR;
    }

    lt_ctx_openssl_t *_ctx = (lt_ctx_openssl_t *)ctx;
    unsigned long err_code;

    // Initialize AES-GCM decryption context.
    _ctx->aesgcm_decrypt_ctx = EVP_CIPHER_CTX_new();
    if (!_ctx->aesgcm_decrypt_ctx) {
        err_code = ERR_get_error();
        LT_LOG_ERROR("Failed to allocate AES-GCM decryption context, err_code=%lu (%s)", err_code,
                     ERR_error_string(err_code, NULL));
        return LT_CRYPTO_ERR;
    }

    // Set cipher type.
    if (!EVP_DecryptInit_ex(_ctx->aesgcm_decrypt_ctx, EVP_aes_256_gcm(), NULL, NULL, NULL)) {
        err_code = ERR_get_error();
        LT_LOG_ERROR(
            "Failed to initialize AES-GCM decryption context with cipher type, err_code=%lu (%s)",
            err_code, ERR_error_string(err_code, NULL));
        return LT_CRYPTO_ERR;
    }

    // Set IV length.
    if (!EVP_CIPHER_CTX_ctrl(_ctx->aesgcm_decrypt_ctx, EVP_CTRL_GCM_SET_IVLEN, TR01_L3_IV_SIZE,
                             NULL)) {
        err_code = ERR_get_error();
        LT_LOG_ERROR(
            "Failed to initialize AES-GCM decryption context with IV length, err_code=%lu (%s)",
            err_code, ERR_error_string(err_code, NULL));
        return LT_CRYPTO_ERR;
    }

    // Set decryption key.
    if (!EVP_DecryptInit_ex(_ctx->aesgcm_decrypt_ctx, NULL, NULL, key, NULL)) {
        err_code = ERR_get_error();
        LT_LOG_ERROR("Failed to initialize AES-GCM decryption context with key, err_code=%lu (%s)",
                     err_code, ERR_error_string(err_code, NULL));
        return LT_CRYPTO_ERR;
    }

    return LT_OK;
}

lt_ret_t lt_aesgcm_encrypt(void *ctx, const uint8_t *iv, const uint32_t iv_len, const uint8_t *add,
                           const uint32_t add_len, const uint8_t *plaintext,
                           const uint32_t plaintext_len, uint8_t *ciphertext,
                           const uint32_t ciphertext_len)
{
    if (iv_len != TR01_L3_IV_SIZE) {
        LT_LOG_ERROR("Invalid AES-GCM IV length: got %" PRIu32 " bytes, expected %d bytes", iv_len,
                     TR01_L3_IV_SIZE);
        return LT_PARAM_ERR;
    }

    lt_ctx_openssl_t *_ctx = (lt_ctx_openssl_t *)ctx;
    unsigned long err_code;
    int out_len;

    // Set IV.
    if (!EVP_EncryptInit_ex(_ctx->aesgcm_encrypt_ctx, NULL, NULL, NULL, iv)) {
        err_code = ERR_get_error();
        LT_LOG_ERROR("Failed to set AES-GCM encryption IV, err_code=%lu (%s)", err_code,
                     ERR_error_string(err_code, NULL));
        return LT_CRYPTO_ERR;
    }

    // Process AAD (Additional Authenticated Data).
    if (!EVP_EncryptUpdate(_ctx->aesgcm_encrypt_ctx, NULL, &out_len, add, (int)add_len)) {
        err_code = ERR_get_error();
        LT_LOG_ERROR("Failed to process AES-GCM AAD, err_code=%lu (%s)", err_code,
                     ERR_error_string(err_code, NULL));
        return LT_CRYPTO_ERR;
    }

    // Encrypt plaintext.
    if (!EVP_EncryptUpdate(_ctx->aesgcm_encrypt_ctx, ciphertext, &out_len, plaintext,
                           (int)plaintext_len)) {
        err_code = ERR_get_error();
        LT_LOG_ERROR("Failed to encrypt AES-GCM plaintext, err_code=%lu (%s)", err_code,
                     ERR_error_string(err_code, NULL));
        return LT_CRYPTO_ERR;
    }

    // Check that all plaintext data was processed.
    if (out_len != (int)(ciphertext_len - TR01_L3_TAG_SIZE)) {
        LT_LOG_ERROR("AES-GCM encryption length mismatch! Current: %d bytes, expected: %" PRIu32
                     " bytes",
                     out_len, ciphertext_len - TR01_L3_TAG_SIZE);
        return LT_CRYPTO_ERR;
    }

    // Finalize encryption.
    if (!EVP_EncryptFinal_ex(_ctx->aesgcm_encrypt_ctx, ciphertext + out_len, &out_len)) {
        err_code = ERR_get_error();
        LT_LOG_ERROR("Failed to finalize AES-GCM encryption, err_code=%lu (%s)", err_code,
                     ERR_error_string(err_code, NULL));
        return LT_CRYPTO_ERR;
    }

    // Get the tag.
    if (!EVP_CIPHER_CTX_ctrl(_ctx->aesgcm_encrypt_ctx, EVP_CTRL_GCM_GET_TAG, TR01_L3_TAG_SIZE,
                             ciphertext + plaintext_len)) {
        err_code = ERR_get_error();
        LT_LOG_ERROR("Failed to get AES-GCM encryption tag, err_code=%lu (%s)", err_code,
                     ERR_error_string(err_code, NULL));
        return LT_CRYPTO_ERR;
    }

    return LT_OK;
}

lt_ret_t lt_aesgcm_decrypt(void *ctx, const uint8_t *iv, const uint32_t iv_len, const uint8_t *add,
                           const uint32_t add_len, const uint8_t *ciphertext,
                           const uint32_t ciphertext_len, uint8_t *plaintext,
                           const uint32_t plaintext_len)
{
    if (iv_len != TR01_L3_IV_SIZE) {
        LT_LOG_ERROR("Invalid AES-GCM IV length: got %" PRIu32 " bytes, expected %d bytes", iv_len,
                     TR01_L3_IV_SIZE);
        return LT_PARAM_ERR;
    }

    lt_ctx_openssl_t *_ctx = (lt_ctx_openssl_t *)ctx;
    unsigned long err_code;
    int out_len;

    // Set IV.
    if (!EVP_DecryptInit_ex(_ctx->aesgcm_decrypt_ctx, NULL, NULL, NULL, iv)) {
        err_code = ERR_get_error();
        LT_LOG_ERROR("Failed to set AES-GCM decryption IV, err_code=%lu (%s)", err_code,
                     ERR_error_string(err_code, NULL));
        return LT_CRYPTO_ERR;
    }

    // Process AAD (Additional Authenticated Data).
    if (!EVP_DecryptUpdate(_ctx->aesgcm_decrypt_ctx, NULL, &out_len, add, (int)add_len)) {
        err_code = ERR_get_error();
        LT_LOG_ERROR("Failed to process AES-GCM AAD, err_code=%lu (%s)", err_code,
                     ERR_error_string(err_code, NULL));
        return LT_CRYPTO_ERR;
    }

    // Decrypt ciphertext.
    if (!EVP_DecryptUpdate(_ctx->aesgcm_decrypt_ctx, plaintext, &out_len, ciphertext,
                           (int)(ciphertext_len - TR01_L3_TAG_SIZE))) {
        err_code = ERR_get_error();
        LT_LOG_ERROR("Failed to decrypt AES-GCM ciphertext, err_code=%lu (%s)", err_code,
                     ERR_error_string(err_code, NULL));
        return LT_CRYPTO_ERR;
    }

    // Check that all ciphertext data was processed.
    if (out_len != (int)(plaintext_len)) {
        LT_LOG_ERROR("AES-GCM decryption length mismatch! Current: %d bytes, expected: %" PRIu32
                     " bytes",
                     out_len, plaintext_len);
        return LT_CRYPTO_ERR;
    }

    // Set expected tag value.
    if (!EVP_CIPHER_CTX_ctrl(_ctx->aesgcm_decrypt_ctx, EVP_CTRL_GCM_SET_TAG, TR01_L3_TAG_SIZE,
                             (void *)(ciphertext + ciphertext_len - TR01_L3_TAG_SIZE))) {
        err_code = ERR_get_error();
        LT_LOG_ERROR("Failed to set AES-GCM decryption tag, err_code=%lu (%s)", err_code,
                     ERR_error_string(err_code, NULL));
        return LT_CRYPTO_ERR;
    }

    // Finalize decryption.
    if (EVP_DecryptFinal_ex(_ctx->aesgcm_decrypt_ctx, plaintext + out_len, &out_len) <= 0) {
        err_code = ERR_get_error();
        LT_LOG_ERROR("Failed to finalize AES-GCM decryption, err_code=%lu (%s)", err_code,
                     ERR_error_string(err_code, NULL));
        return LT_CRYPTO_ERR;
    }

    return LT_OK;
}

lt_ret_t lt_aesgcm_encrypt_deinit(void *ctx)
{
    lt_ctx_openssl_t *_ctx = (lt_ctx_openssl_t *)ctx;

    EVP_CIPHER_CTX_free(_ctx->aesgcm_encrypt_ctx);
    _ctx->aesgcm_encrypt_ctx = NULL;

    return LT_OK;
}

lt_ret_t lt_aesgcm_decrypt_deinit(void *ctx)
{
    lt_ctx_openssl_t *_ctx = (lt_ctx_openssl_t *)ctx;

    EVP_CIPHER_CTX_free(_ctx->aesgcm_decrypt_ctx);
    _ctx->aesgcm_decrypt_ctx = NULL;

    return LT_OK;
}