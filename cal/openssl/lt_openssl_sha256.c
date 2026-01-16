/**
 * @file lt_openssl_sha256.c
 * @copyright Copyright (c) 2020-2025 Tropic Square s.r.o.
 *
 * @license For the license see LICENSE.md in the root directory of this source tree.
 */

#include <inttypes.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <stdint.h>
#include <string.h>

#include "libtropic_common.h"
#include "libtropic_logging.h"
#include "libtropic_openssl.h"
#include "lt_sha256.h"

lt_ret_t lt_sha256_init(void *ctx)
{
    lt_ctx_openssl_t *_ctx = (lt_ctx_openssl_t *)ctx;
    unsigned long err_code;

    _ctx->sha256_ctx = EVP_MD_CTX_new();
    if (!_ctx->sha256_ctx) {
        err_code = ERR_get_error();
        LT_LOG_ERROR("Failed to allocate SHA-256 context, err_code=%lu (%s)", err_code,
                     ERR_error_string(err_code, NULL));
        return LT_CRYPTO_ERR;
    }

    return LT_OK;
}

lt_ret_t lt_sha256_start(void *ctx)
{
    lt_ctx_openssl_t *_ctx = (lt_ctx_openssl_t *)ctx;
    unsigned long err_code;

    if (!EVP_DigestInit_ex(_ctx->sha256_ctx, EVP_sha256(), NULL)) {
        err_code = ERR_get_error();
        LT_LOG_ERROR("Failed to initialize SHA-256 context with hash type, err_code=%lu (%s)", err_code,
                     ERR_error_string(err_code, NULL));
        return LT_CRYPTO_ERR;
    }

    return LT_OK;
}

lt_ret_t lt_sha256_update(void *ctx, const uint8_t *input, const size_t input_len)
{
    lt_ctx_openssl_t *_ctx = (lt_ctx_openssl_t *)ctx;
    unsigned long err_code;

    if (!EVP_DigestUpdate(_ctx->sha256_ctx, input, input_len)) {
        err_code = ERR_get_error();
        LT_LOG_ERROR("Failed to update SHA-256 hash, err_code=%lu (%s)", err_code, ERR_error_string(err_code, NULL));
        return LT_CRYPTO_ERR;
    }

    return LT_OK;
}

lt_ret_t lt_sha256_finish(void *ctx, uint8_t *output)
{
    lt_ctx_openssl_t *_ctx = (lt_ctx_openssl_t *)ctx;
    unsigned long err_code;

    if (!EVP_DigestFinal_ex(_ctx->sha256_ctx, output, NULL)) {
        err_code = ERR_get_error();
        LT_LOG_ERROR("Failed to finalize SHA-256 hash, err_code=%lu (%s)", err_code, ERR_error_string(err_code, NULL));
        return LT_CRYPTO_ERR;
    }

    return LT_OK;
}

lt_ret_t lt_sha256_deinit(void *ctx)
{
    lt_ctx_openssl_t *_ctx = (lt_ctx_openssl_t *)ctx;

    EVP_MD_CTX_free(_ctx->sha256_ctx);
    _ctx->sha256_ctx = NULL;

    return LT_OK;
}