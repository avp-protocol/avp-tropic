/**
 * @file lt_wolfcrypt_hmac_sha256.c
 * @copyright Copyright (c) 2020-2025 Tropic Square s.r.o.
 *
 * @license For the license see LICENSE.md in the root directory of this source tree.
 */

#include <inttypes.h>
#include <stdint.h>
#include <wolfssl/wolfcrypt/error-crypt.h>
#include <wolfssl/wolfcrypt/hmac.h>

#include "libtropic_common.h"
#include "libtropic_logging.h"
#include "lt_hmac_sha256.h"

lt_ret_t lt_hmac_sha256(const uint8_t *key, const uint32_t key_len, const uint8_t *input, const uint32_t input_len,
                        uint8_t *output)
{
    int ret;
    Hmac hmac;

    // Initialize HMAC context
    ret = wc_HmacInit(&hmac, NULL, INVALID_DEVID);
    if (ret != 0) {
        LT_LOG_ERROR("Failed to initialize HMAC context, ret=%d (%s)", ret, wc_GetErrorString(ret));
        return LT_CRYPTO_ERR;
    }

    // Set HMAC key
    ret = wc_HmacSetKey(&hmac, WC_SHA256, key, key_len);
    if (ret != 0) {
        LT_LOG_ERROR("Failed to set HMAC key, ret=%d (%s)", ret, wc_GetErrorString(ret));
        wc_HmacFree(&hmac);
        return LT_CRYPTO_ERR;
    }

    // Compute HMAC
    ret = wc_HmacUpdate(&hmac, input, input_len);
    if (ret != 0) {
        LT_LOG_ERROR("HMAC update failed, ret=%d (%s)", ret, wc_GetErrorString(ret));
        wc_HmacFree(&hmac);
        return LT_CRYPTO_ERR;
    }

    ret = wc_HmacFinal(&hmac, output);
    if (ret != 0) {
        LT_LOG_ERROR("HMAC finalization failed, ret=%d (%s)", ret, wc_GetErrorString(ret));
        wc_HmacFree(&hmac);
        return LT_CRYPTO_ERR;
    }

    // Free HMAC context
    wc_HmacFree(&hmac);

    return LT_OK;
}
