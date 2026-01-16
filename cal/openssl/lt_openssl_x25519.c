/**
 * @file lt_openssl_x25519.c
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
#include "lt_x25519.h"

lt_ret_t lt_X25519(const uint8_t *privkey, const uint8_t *pubkey, uint8_t *secret)
{
    EVP_PKEY_CTX *ctx = NULL;
    EVP_PKEY *priv = NULL;
    EVP_PKEY *pub = NULL;
    lt_ret_t lt_ret = LT_OK;
    unsigned long err_code;

    // Create private key EVP_PKEY structure.
    priv = EVP_PKEY_new_raw_private_key(EVP_PKEY_X25519, NULL, privkey, TR01_X25519_KEY_LEN);
    if (!priv) {
        err_code = ERR_get_error();
        LT_LOG_ERROR("Failed to create private key EVP_PKEY structure, err_code=%lu (%s)", err_code,
                     ERR_error_string(err_code, NULL));
        lt_ret = LT_CRYPTO_ERR;
        goto lt_X25519_cleanup;
    }

    // Create public key EVP_PKEY structure.
    pub = EVP_PKEY_new_raw_public_key(EVP_PKEY_X25519, NULL, pubkey, TR01_X25519_KEY_LEN);
    if (!pub) {
        err_code = ERR_get_error();
        LT_LOG_ERROR("Failed to create public key EVP_PKEY structure, err_code=%lu (%s)", err_code,
                     ERR_error_string(err_code, NULL));
        lt_ret = LT_CRYPTO_ERR;
        goto lt_X25519_cleanup;
    }

    // Create the context for the shared secret derivation.
    ctx = EVP_PKEY_CTX_new(priv, NULL);
    if (!ctx) {
        err_code = ERR_get_error();
        LT_LOG_ERROR("Failed to create EVP_PKEY_CTX for X25519, err_code=%lu (%s)", err_code,
                     ERR_error_string(err_code, NULL));
        lt_ret = LT_CRYPTO_ERR;
        goto lt_X25519_cleanup;
    }

    // Initialize the derivation.
    if (EVP_PKEY_derive_init(ctx) <= 0) {
        err_code = ERR_get_error();
        LT_LOG_ERROR("Failed to initialize EVP_PKEY_CTX for X25519 derivation, err_code=%lu (%s)", err_code,
                     ERR_error_string(err_code, NULL));
        lt_ret = LT_CRYPTO_ERR;
        goto lt_X25519_cleanup;
    }

    // Provide the peer public key.
    if (EVP_PKEY_derive_set_peer(ctx, pub) <= 0) {
        err_code = ERR_get_error();
        LT_LOG_ERROR("Failed to set peer public key for X25519 derivation, err_code=%lu (%s)", err_code,
                     ERR_error_string(err_code, NULL));
        lt_ret = LT_CRYPTO_ERR;
        goto lt_X25519_cleanup;
    }

    // Derive the shared secret.
    size_t secret_len = TR01_X25519_KEY_LEN;
    if (EVP_PKEY_derive(ctx, secret, &secret_len) <= 0) {
        err_code = ERR_get_error();
        LT_LOG_ERROR("Failed to derive X25519 shared secret, err_code=%lu (%s)", err_code,
                     ERR_error_string(err_code, NULL));
        lt_ret = LT_CRYPTO_ERR;
        goto lt_X25519_cleanup;
    }

    if (secret_len != TR01_X25519_KEY_LEN) {
        LT_LOG_ERROR("X25519 derived secret length mismatch! Current: %zu bytes, expected: %d bytes", secret_len,
                     TR01_X25519_KEY_LEN);
        lt_ret = LT_CRYPTO_ERR;
        goto lt_X25519_cleanup;
    }

lt_X25519_cleanup:
    EVP_PKEY_free(priv);
    EVP_PKEY_free(pub);
    EVP_PKEY_CTX_free(ctx);
    return lt_ret;
}

lt_ret_t lt_X25519_scalarmult(const uint8_t *sk, uint8_t *pk)
{
    EVP_PKEY *priv = NULL;
    lt_ret_t lt_ret = LT_OK;
    unsigned long err_code;

    // Create private key EVP_PKEY structure.
    priv = EVP_PKEY_new_raw_private_key(EVP_PKEY_X25519, NULL, sk, TR01_X25519_KEY_LEN);
    if (!priv) {
        err_code = ERR_get_error();
        LT_LOG_ERROR("Failed to create private key EVP_PKEY structure, err_code=%lu (%s)", err_code,
                     ERR_error_string(err_code, NULL));
        lt_ret = LT_CRYPTO_ERR;
        goto lt_X25519_scalarmult_cleanup;
    }

    // Extract the public key.
    size_t pk_len = TR01_X25519_KEY_LEN;
    if (!EVP_PKEY_get_raw_public_key(priv, pk, &pk_len)) {
        err_code = ERR_get_error();
        LT_LOG_ERROR("Failed to extract X25519 public key from private key, err_code=%lu (%s)", err_code,
                     ERR_error_string(err_code, NULL));
        lt_ret = LT_CRYPTO_ERR;
        goto lt_X25519_scalarmult_cleanup;
    }

    if (pk_len != TR01_X25519_KEY_LEN) {
        LT_LOG_ERROR("X25519 public key length mismatch! Current: %zu bytes, expected: %d bytes", pk_len,
                     TR01_X25519_KEY_LEN);
        lt_ret = LT_CRYPTO_ERR;
        goto lt_X25519_scalarmult_cleanup;
    }

lt_X25519_scalarmult_cleanup:
    EVP_PKEY_free(priv);
    return lt_ret;
}
