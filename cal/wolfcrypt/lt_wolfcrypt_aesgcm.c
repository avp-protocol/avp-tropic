/**
 * @file lt_wolfcrypt_aesgcm.c
 * @copyright Copyright (c) 2020-2026 Tropic Square s.r.o.
 *
 * @license For the license see LICENSE.md in the root directory of this source tree.
 */

#include <inttypes.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <wolfssl/wolfcrypt/aes.h>
#include <wolfssl/wolfcrypt/error-crypt.h>

#include "libtropic_common.h"
#include "libtropic_logging.h"
#include "libtropic_wolfcrypt.h"
#include "lt_aesgcm.h"

/**
 * @brief Initializes WolfCrypt AES-GCM context.
 *
 * @param ctx      AES-GCM context structure (WolfCrypt specific)
 * @param heap     Heap hint to use for malloc / free if needed
 * @param dev_id   ID to use with crypto callbacks or async hardware
 * @param key      Key to initialize with
 * @param key_len  Length of the key
 * @return         LT_OK if success, otherwise returns other error code.
 */
static lt_ret_t lt_aesgcm_init(lt_aesgcm_ctx_wolfcrypt_t *ctx, void *heap, const int dev_id,
                               const uint8_t *key, const uint32_t key_len)
{
    int ret;

    if (ctx->initialized) {
        LT_LOG_ERROR("AES-GCM context already initialized!");
        return LT_CRYPTO_ERR;
    }

    ret = wc_AesInit(&ctx->ctx, heap, dev_id);
    if (ret != 0) {
        LT_LOG_ERROR("Failed to init AES-GCM context, ret=%d (%s)", ret, wc_GetErrorString(ret));
        return LT_CRYPTO_ERR;
    }

    ret = wc_AesGcmSetKey(&ctx->ctx, key, key_len);
    if (ret != 0) {
        LT_LOG_ERROR("Failed to set AES-GCM key, ret=%d (%s)", ret, wc_GetErrorString(ret));
        return LT_CRYPTO_ERR;
    }

    ctx->initialized = true;
    return LT_OK;
}

/**
 * @brief Deinitializes WolfCrypt AES-GCM context.
 *
 * @param ctx  AES-GCM context structure (WolfCrypt specific)
 * @return     LT_OK if success, otherwise returns other error code.
 */
static lt_ret_t lt_aesgcm_deinit(lt_aesgcm_ctx_wolfcrypt_t *ctx)
{
    if (ctx->initialized) {
        wc_AesFree(&ctx->ctx);
        ctx->initialized = false;
    }

    return LT_OK;
}

lt_ret_t lt_aesgcm_encrypt_init(void *ctx, const uint8_t *key, const uint32_t key_len)
{
    lt_ctx_wolfcrypt_t *_ctx = (lt_ctx_wolfcrypt_t *)ctx;

    return lt_aesgcm_init(&_ctx->aesgcm_encrypt_ctx,
                          NULL,           // don't use heap
                          INVALID_DEVID,  // don't use device ID
                          key, key_len);
}

lt_ret_t lt_aesgcm_decrypt_init(void *ctx, const uint8_t *key, const uint32_t key_len)
{
    lt_ctx_wolfcrypt_t *_ctx = (lt_ctx_wolfcrypt_t *)ctx;

    return lt_aesgcm_init(&_ctx->aesgcm_decrypt_ctx,
                          NULL,           // don't use heap
                          INVALID_DEVID,  // don't use device ID
                          key, key_len);
}

lt_ret_t lt_aesgcm_encrypt(void *ctx, const uint8_t *iv, const uint32_t iv_len, const uint8_t *add,
                           const uint32_t add_len, const uint8_t *plaintext,
                           const uint32_t plaintext_len, uint8_t *ciphertext,
                           const uint32_t ciphertext_len)
{
    lt_ctx_wolfcrypt_t *_ctx = (lt_ctx_wolfcrypt_t *)ctx;

    int ret = wc_AesGcmEncrypt(&_ctx->aesgcm_encrypt_ctx.ctx, ciphertext, plaintext, plaintext_len, iv,
                               iv_len, ciphertext + (ciphertext_len - TR01_L3_TAG_SIZE),
                               TR01_L3_TAG_SIZE, add, add_len);
    if (ret != 0) {
        LT_LOG_ERROR("AES-GCM encryption failed, ret=%d (%s)", ret, wc_GetErrorString(ret));
        return LT_CRYPTO_ERR;
    }

    return LT_OK;
}

lt_ret_t lt_aesgcm_decrypt(void *ctx, const uint8_t *iv, const uint32_t iv_len, const uint8_t *add,
                           const uint32_t add_len, const uint8_t *ciphertext,
                           const uint32_t ciphertext_len, uint8_t *plaintext,
                           const uint32_t plaintext_len)
{
    lt_ctx_wolfcrypt_t *_ctx = (lt_ctx_wolfcrypt_t *)ctx;

    int ret = wc_AesGcmDecrypt(&_ctx->aesgcm_decrypt_ctx.ctx, plaintext, ciphertext, plaintext_len, iv,
                               iv_len, ciphertext + (ciphertext_len - TR01_L3_TAG_SIZE),
                               TR01_L3_TAG_SIZE, add, add_len);
    if (ret != 0) {
        LT_LOG_ERROR("AES-GCM decryption failed, ret=%d (%s)", ret, wc_GetErrorString(ret));
        return LT_CRYPTO_ERR;
    }

    return LT_OK;
}

lt_ret_t lt_aesgcm_encrypt_deinit(void *ctx)
{
    lt_ctx_wolfcrypt_t *_ctx = (lt_ctx_wolfcrypt_t *)ctx;

    return lt_aesgcm_deinit(&_ctx->aesgcm_encrypt_ctx);
}

lt_ret_t lt_aesgcm_decrypt_deinit(void *ctx)
{
    lt_ctx_wolfcrypt_t *_ctx = (lt_ctx_wolfcrypt_t *)ctx;

    return lt_aesgcm_deinit(&_ctx->aesgcm_decrypt_ctx);
}