/**
 * @file lt_wolfcrypt_x25519.c
 * @copyright Copyright (c) 2020-2026 Tropic Square s.r.o.
 *
 * @license For the license see LICENSE.md in the root directory of this source tree.
 */

#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <wolfssl/wolfcrypt/curve25519.h>
#include <wolfssl/wolfcrypt/error-crypt.h>
#include <wolfssl/wolfcrypt/random.h>

#include "libtropic_common.h"
#include "libtropic_logging.h"
#include "lt_x25519.h"

lt_ret_t lt_X25519(const uint8_t *privkey, const uint8_t *pubkey, uint8_t *secret)
{
    int ret;
    lt_ret_t lt_ret = LT_OK;
    curve25519_key wc_priv, wc_pub;
#ifdef WOLFSSL_CURVE25519_BLINDING
    WC_RNG rng;
    bool rng_initialized = false;
#endif

    ret = wc_curve25519_init(&wc_priv);
    if (ret != 0) {
        LT_LOG_ERROR("Failed to initialize X25519 private key, ret=%d (%s)", ret, wc_GetErrorString(ret));
        return LT_CRYPTO_ERR;
    }

    ret = wc_curve25519_init(&wc_pub);
    if (ret != 0) {
        LT_LOG_ERROR("Failed to initialize X25519 public key, ret=%d (%s)", ret, wc_GetErrorString(ret));
        lt_ret = LT_CRYPTO_ERR;
        goto lt_X25519_cleanup;
    }

    ret = wc_curve25519_import_private_ex(privkey, TR01_X25519_KEY_LEN, &wc_priv, EC25519_LITTLE_ENDIAN);
    if (ret != 0) {
        LT_LOG_ERROR("Failed to import X25519 private key, ret=%d (%s)", ret, wc_GetErrorString(ret));
        lt_ret = LT_CRYPTO_ERR;
        goto lt_X25519_cleanup;
    }

#ifdef WOLFSSL_CURVE25519_BLINDING
    ret = wc_InitRng(&rng);
    if (ret != 0) {
        LT_LOG_ERROR("Failed to init RNG for X25519 blinding, ret=%d (%s)", ret, wc_GetErrorString(ret));
        lt_ret = LT_CRYPTO_ERR;
        goto lt_X25519_cleanup;
    }
    rng_initialized = true;

    ret = wc_curve25519_set_rng(&wc_priv, &rng);
    if (ret != 0) {
        LT_LOG_ERROR("Failed to set RNG for X25519 key blinding, ret=%d (%s)", ret, wc_GetErrorString(ret));
        lt_ret = LT_CRYPTO_ERR;
        goto lt_X25519_cleanup;
    }
#endif

    ret = wc_curve25519_import_public_ex(pubkey, TR01_X25519_KEY_LEN, &wc_pub, EC25519_LITTLE_ENDIAN);
    if (ret != 0) {
        LT_LOG_ERROR("Failed to import X25519 public key, ret=%d (%s)", ret, wc_GetErrorString(ret));
        lt_ret = LT_CRYPTO_ERR;
        goto lt_X25519_cleanup;
    }

    word32 secret_out_len = TR01_X25519_KEY_LEN;
    ret = wc_curve25519_shared_secret_ex(&wc_priv, &wc_pub, secret, &secret_out_len, EC25519_LITTLE_ENDIAN);
    if (ret != 0) {
        LT_LOG_ERROR("Failed to compute X25519 shared secret key, ret=%d (%s)", ret, wc_GetErrorString(ret));
        lt_ret = LT_CRYPTO_ERR;
        goto lt_X25519_cleanup;
    }

    if (secret_out_len != TR01_X25519_KEY_LEN) {
        LT_LOG_ERROR("X25519 shared secret key has unexpected length: %u", secret_out_len);
        lt_ret = LT_CRYPTO_ERR;
        goto lt_X25519_cleanup;
    }

lt_X25519_cleanup:
#ifdef WOLFSSL_CURVE25519_BLINDING
    if (rng_initialized) {
        ret = wc_FreeRng(&rng);
        if (ret != 0) {
            LT_LOG_WARN("Failed to free RNG used for X25519 blinding, ret=%d (%s)", ret, wc_GetErrorString(ret));
        }
    }
#endif
    wc_curve25519_free(&wc_priv);
    wc_curve25519_free(&wc_pub);
    return lt_ret;
}

lt_ret_t lt_X25519_scalarmult(const uint8_t *sk, uint8_t *pk)
{
    int ret;
    lt_ret_t lt_ret = LT_OK;
    curve25519_key wc_secret;
#ifdef WOLFSSL_CURVE25519_BLINDING
    WC_RNG rng;
    bool rng_initialized = false;
#endif

    ret = wc_curve25519_init(&wc_secret);
    if (ret != 0) {
        LT_LOG_ERROR("Failed to initialize X25519 private key, ret=%d (%s)", ret, wc_GetErrorString(ret));
        return LT_CRYPTO_ERR;
    }

    ret = wc_curve25519_import_private_ex(sk, TR01_X25519_KEY_LEN, &wc_secret, EC25519_LITTLE_ENDIAN);
    if (ret != 0) {
        LT_LOG_ERROR("Failed to import X25519 private key, ret=%d (%s)", ret, wc_GetErrorString(ret));
        lt_ret = LT_CRYPTO_ERR;
        goto lt_X25519_scalarmult_cleanup;
    }

#ifdef WOLFSSL_CURVE25519_BLINDING
    ret = wc_InitRng(&rng);
    if (ret != 0) {
        LT_LOG_ERROR("Failed to init RNG for X25519 blinding, ret=%d (%s)", ret, wc_GetErrorString(ret));
        lt_ret = LT_CRYPTO_ERR;
        goto lt_X25519_scalarmult_cleanup;
    }
    rng_initialized = true;

    ret = wc_curve25519_set_rng(&wc_secret, &rng);
    if (ret != 0) {
        LT_LOG_ERROR("Failed to set RNG for X25519 key blinding, ret=%d (%s)", ret, wc_GetErrorString(ret));
        lt_ret = LT_CRYPTO_ERR;
        goto lt_X25519_scalarmult_cleanup;
    }
#endif

    word32 pk_out_len = TR01_X25519_KEY_LEN;
    ret = wc_curve25519_export_public_ex(&wc_secret, pk, &pk_out_len, EC25519_LITTLE_ENDIAN);
    if (ret != 0) {
        LT_LOG_ERROR("Failed to compute X25519 public key, ret=%d (%s)", ret, wc_GetErrorString(ret));
        lt_ret = LT_CRYPTO_ERR;
        goto lt_X25519_scalarmult_cleanup;
    }

    if (pk_out_len != TR01_X25519_KEY_LEN) {
        LT_LOG_ERROR("X25519 public key has unexpected length: %u", pk_out_len);
        lt_ret = LT_CRYPTO_ERR;
        goto lt_X25519_scalarmult_cleanup;
    }

lt_X25519_scalarmult_cleanup:
#ifdef WOLFSSL_CURVE25519_BLINDING
    if (rng_initialized) {
        ret = wc_FreeRng(&rng);
        if (ret != 0) {
            LT_LOG_WARN("Failed to free RNG used for X25519 blinding, ret=%d (%s)", ret, wc_GetErrorString(ret));
        }
    }
#endif
    wc_curve25519_free(&wc_secret);
    return lt_ret;
}