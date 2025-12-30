/**
 * @file lt_wolfcrypt_sha256.c
 * @copyright Copyright (c) 2020-2025 Tropic Square s.r.o.
 *
 * @license For the license see LICENSE.md in the root directory of this source tree.
 */

#include <inttypes.h>
#include <stdint.h>
#include <string.h>
#include <wolfssl/wolfcrypt/error-crypt.h>
#include <wolfssl/wolfcrypt/sha256.h>

#include "libtropic_common.h"
#include "libtropic_logging.h"
#include "libtropic_wolfcrypt.h"
#include "lt_sha256.h"

lt_ret_t lt_sha256_init(void *ctx)
{
    lt_ctx_wolfcrypt_t *_ctx = (lt_ctx_wolfcrypt_t *)ctx;

    int ret = wc_InitSha256(&_ctx->sha256_ctx);
    if (ret != 0) {
        LT_LOG_ERROR("SHA-256 init failed, ret=%d (%s)", ret, wc_GetErrorString(ret));
        return LT_CRYPTO_ERR;
    }

    return LT_OK;
}

lt_ret_t lt_sha256_start(void *ctx)
{
    // Do nothing, as wc_InitSha256 (called by lt_sha256_init)
    // already prepares the context for a new hash computation.
    LT_UNUSED(ctx);
    return LT_OK;
}

lt_ret_t lt_sha256_update(void *ctx, const uint8_t *input, const size_t input_len)
{
    lt_ctx_wolfcrypt_t *_ctx = (lt_ctx_wolfcrypt_t *)ctx;

    int ret = wc_Sha256Update(&_ctx->sha256_ctx, input, input_len);
    if (ret != 0) {
        LT_LOG_ERROR("SHA-256 update failed, ret=%d (%s)", ret, wc_GetErrorString(ret));
        return LT_CRYPTO_ERR;
    }

    return LT_OK;
}

lt_ret_t lt_sha256_finish(void *ctx, uint8_t *output)
{
    lt_ctx_wolfcrypt_t *_ctx = (lt_ctx_wolfcrypt_t *)ctx;
    lt_ret_t lt_ret = LT_OK;

    int ret = wc_Sha256Final(&_ctx->sha256_ctx, output);
    if (ret != 0) {
        LT_LOG_ERROR("SHA-256 finish failed, ret=%d (%s)", ret, wc_GetErrorString(ret));
        lt_ret = LT_CRYPTO_ERR;
    }

    return lt_ret;
}

lt_ret_t lt_sha256_deinit(void *ctx)
{
    lt_ctx_wolfcrypt_t *_ctx = (lt_ctx_wolfcrypt_t *)ctx;

    wc_Sha256Free(&_ctx->sha256_ctx);
    return LT_OK;
}