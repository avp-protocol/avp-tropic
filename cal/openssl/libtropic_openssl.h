#ifndef LT_OPENSSL_H
#define LT_OPENSSL_H

/**
 * @file lt_openssl.h
 * @brief OpenSSL public declarations.
 * @copyright Copyright (c) 2020-2025 Tropic Square s.r.o.
 *
 * @license For the license see LICENSE.md in the root directory of this source tree.
 */

#include <openssl/evp.h>

/**
 * @brief Context structure for OpenSSL.
 *
 */
typedef struct lt_ctx_openssl_t {
    /** @private @brief AES-GCM context for encryption. */
    EVP_CIPHER_CTX *aesgcm_encrypt_ctx;
    /** @private @brief AES-GCM context for decryption. */
    EVP_CIPHER_CTX *aesgcm_decrypt_ctx;
    /** @private @brief SHA-256 context. */
    EVP_MD_CTX *sha256_ctx;
} lt_ctx_openssl_t;

#endif  // LT_OPENSSL_H