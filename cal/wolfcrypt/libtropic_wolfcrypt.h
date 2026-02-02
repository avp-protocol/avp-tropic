#ifndef LT_WOLFCRYPT_H
#define LT_WOLFCRYPT_H

/**
 * @file libtropic_wolfcrypt.h
 * @brief WolfCrypt public declarations.
 * @copyright Copyright (c) 2020-2026 Tropic Square s.r.o.
 *
 * @license For the license see LICENSE.md in the root directory of this source tree.
 */

#include <wolfssl/wolfcrypt/aes.h>
#include <wolfssl/wolfcrypt/sha256.h>

/**
 * @brief AES-GCM context structure for WolfCrypt.
 *
 */
typedef struct lt_aesgcm_ctx_wolfcrypt_t {
    /** @private @brief AES-GCM context. */
    Aes ctx;
    /** @private @brief Initialization flag. */
    bool initialized;
} lt_aesgcm_ctx_wolfcrypt_t;

/**
 * @brief Context structure for WolfCrypt.
 *
 */
typedef struct lt_ctx_wolfcrypt_t {
    /** @private @brief AES-GCM context for encryption. */
    lt_aesgcm_ctx_wolfcrypt_t aesgcm_encrypt_ctx;
    /** @private @brief AES-GCM context for decryption. */
    lt_aesgcm_ctx_wolfcrypt_t aesgcm_decrypt_ctx;
    /** @private @brief SHA-256 context. */
    wc_Sha256 sha256_ctx;
} lt_ctx_wolfcrypt_t;

#endif  // LT_WOLFCRYPT_H