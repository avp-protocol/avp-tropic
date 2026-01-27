/**
 * @file lt_wolfcrypt_common.c
 * @copyright Copyright (c) 2020-2026 Tropic Square s.r.o.
 *
 * @license For the license see LICENSE.md in the root directory of this source tree.
 */

#include <stdbool.h>

#include "libtropic_logging.h"
#include "libtropic_wolfcrypt.h"
#include "lt_aesgcm.h"
#include "lt_crypto_common.h"
#include "lt_sha256.h"

lt_ret_t lt_crypto_ctx_init(void *ctx)
{
    lt_ctx_wolfcrypt_t *_ctx = (lt_ctx_wolfcrypt_t *)ctx;

    _ctx->aesgcm_encrypt_ctx.initialized = false;
    _ctx->aesgcm_decrypt_ctx.initialized = false;

    return LT_OK;
}

lt_ret_t lt_crypto_ctx_deinit(void *ctx)
{
    lt_ret_t ret1 = lt_aesgcm_encrypt_deinit(ctx);
    lt_ret_t ret2 = lt_aesgcm_decrypt_deinit(ctx);

    if (ret1 != LT_OK) {
        return ret1;
    }
    if (ret2 != LT_OK) {
        return ret2;
    }

    return LT_OK;
}