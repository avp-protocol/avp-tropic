/**
 * @file avp_tropic.h
 * @brief Agent Vault Protocol (AVP) interface for TROPIC01 secure element.
 *
 * This module implements the AVP Hardware conformance level using TROPIC01
 * as the secure backend. It provides:
 *
 * - STORE/RETRIEVE operations via TROPIC01 key slots
 * - HW_CHALLENGE via TROPIC01 device attestation
 * - HW_SIGN via TROPIC01 ECDSA signing (key never leaves silicon)
 * - HW_ATTEST via TROPIC01 certificate chain
 *
 * @copyright Copyright (c) 2026 AVP Protocol Contributors
 * @license Apache-2.0 (see LICENSE)
 */

#ifndef AVP_TROPIC_H
#define AVP_TROPIC_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "libtropic.h"

#ifdef __cplusplus
extern "C" {
#endif

/*=============================================================================
 * AVP Error Codes
 *============================================================================*/

typedef enum {
    AVP_OK = 0,
    AVP_ERR_NOT_INITIALIZED,
    AVP_ERR_AUTHENTICATION_FAILED,
    AVP_ERR_SESSION_EXPIRED,
    AVP_ERR_SECRET_NOT_FOUND,
    AVP_ERR_CAPACITY_EXCEEDED,
    AVP_ERR_INVALID_NAME,
    AVP_ERR_HARDWARE_ERROR,
    AVP_ERR_CRYPTO_ERROR,
    AVP_ERR_INTERNAL,
} avp_ret_t;

/*=============================================================================
 * AVP Session
 *============================================================================*/

/** @brief Maximum secret name length (AVP spec) */
#define AVP_MAX_SECRET_NAME_LEN 255

/** @brief Maximum secret value length (AVP spec: 64KB) */
#define AVP_MAX_SECRET_VALUE_LEN 65536

/** @brief Number of key slots in TROPIC01 for AVP secrets */
#define AVP_TROPIC_KEY_SLOTS 128

/** @brief Session ID prefix as per AVP spec */
#define AVP_SESSION_PREFIX "avp_sess_"

/**
 * @brief AVP session state.
 */
typedef enum {
    AVP_SESSION_INACTIVE = 0,
    AVP_SESSION_ACTIVE,
    AVP_SESSION_EXPIRED,
    AVP_SESSION_TERMINATED,
} avp_session_state_t;

/**
 * @brief AVP vault handle for TROPIC01 backend.
 */
typedef struct avp_vault_t {
    /** @brief libtropic handle */
    lt_handle_t lt_handle;

    /** @brief Session state */
    avp_session_state_t session_state;

    /** @brief Session ID (null-terminated) */
    char session_id[64];

    /** @brief Session creation timestamp (Unix epoch) */
    uint32_t session_created_at;

    /** @brief Session TTL in seconds */
    uint32_t session_ttl;

    /** @brief Workspace name */
    char workspace[256];

    /** @brief Is authenticated */
    bool authenticated;
} avp_vault_t;

/**
 * @brief AVP secret metadata (returned by LIST).
 */
typedef struct avp_secret_metadata_t {
    char name[AVP_MAX_SECRET_NAME_LEN + 1];
    uint32_t created_at;
    uint32_t updated_at;
    uint8_t slot_index;
    uint32_t version;
} avp_secret_metadata_t;

/**
 * @brief AVP discover response.
 */
typedef struct avp_discover_response_t {
    char version[16];
    char conformance[16];  /* "hardware" for TROPIC01 */
    bool attestation;
    bool rotation;
    uint16_t max_secrets;
} avp_discover_response_t;

/**
 * @brief Hardware attestation result.
 */
typedef struct avp_attestation_t {
    bool verified;
    char manufacturer[64];
    char model[64];
    char firmware_version[32];
    char serial[64];
    uint8_t certificate[2048];
    size_t certificate_len;
} avp_attestation_t;

/*=============================================================================
 * AVP Operations
 *============================================================================*/

/**
 * @brief Initialize AVP vault with TROPIC01 backend.
 *
 * @param vault Pointer to vault handle to initialize.
 * @param device Pointer to device-specific structure (e.g., lt_dev_stm32u5_tropic_click_t).
 * @return AVP_OK on success.
 */
avp_ret_t avp_init(avp_vault_t *vault, void *device);

/**
 * @brief Deinitialize AVP vault.
 *
 * @param vault Pointer to vault handle.
 * @return AVP_OK on success.
 */
avp_ret_t avp_deinit(avp_vault_t *vault);

/**
 * @brief DISCOVER operation - query vault capabilities.
 *
 * @param vault Pointer to vault handle.
 * @param response Pointer to response structure to fill.
 * @return AVP_OK on success.
 */
avp_ret_t avp_discover(avp_vault_t *vault, avp_discover_response_t *response);

/**
 * @brief AUTHENTICATE operation - establish session.
 *
 * For TROPIC01, this performs PIN authentication.
 *
 * @param vault Pointer to vault handle.
 * @param workspace Workspace name (or NULL for "default").
 * @param pin PIN string (for hardware backend).
 * @param ttl_seconds Requested session TTL (0 for default).
 * @return AVP_OK on success, AVP_ERR_AUTHENTICATION_FAILED on wrong PIN.
 */
avp_ret_t avp_authenticate(avp_vault_t *vault, const char *workspace, const char *pin, uint32_t ttl_seconds);

/**
 * @brief STORE operation - store a secret.
 *
 * @param vault Pointer to vault handle.
 * @param name Secret name (max 255 chars).
 * @param value Secret value.
 * @param value_len Length of value in bytes.
 * @return AVP_OK on success.
 */
avp_ret_t avp_store(avp_vault_t *vault, const char *name, const uint8_t *value, size_t value_len);

/**
 * @brief RETRIEVE operation - retrieve a secret.
 *
 * @param vault Pointer to vault handle.
 * @param name Secret name.
 * @param value Buffer to receive value.
 * @param value_len Pointer to buffer size (in) / actual size (out).
 * @return AVP_OK on success, AVP_ERR_SECRET_NOT_FOUND if not found.
 */
avp_ret_t avp_retrieve(avp_vault_t *vault, const char *name, uint8_t *value, size_t *value_len);

/**
 * @brief DELETE operation - delete a secret.
 *
 * @param vault Pointer to vault handle.
 * @param name Secret name.
 * @param deleted Set to true if secret existed and was deleted.
 * @return AVP_OK on success.
 */
avp_ret_t avp_delete(avp_vault_t *vault, const char *name, bool *deleted);

/**
 * @brief LIST operation - enumerate secrets.
 *
 * @param vault Pointer to vault handle.
 * @param secrets Array to receive secret metadata.
 * @param max_secrets Maximum number of secrets to return.
 * @param count Pointer to receive actual count.
 * @return AVP_OK on success.
 */
avp_ret_t avp_list(avp_vault_t *vault, avp_secret_metadata_t *secrets, size_t max_secrets, size_t *count);

/*=============================================================================
 * AVP Hardware Extension Operations
 *============================================================================*/

/**
 * @brief HW_CHALLENGE operation - verify device authenticity.
 *
 * @param vault Pointer to vault handle.
 * @param attestation Pointer to attestation result.
 * @return AVP_OK on success.
 */
avp_ret_t avp_hw_challenge(avp_vault_t *vault, avp_attestation_t *attestation);

/**
 * @brief HW_SIGN operation - sign data without exporting key.
 *
 * The signing key never leaves the TROPIC01 secure element.
 *
 * @param vault Pointer to vault handle.
 * @param key_name Name of the signing key.
 * @param data Data to sign.
 * @param data_len Length of data.
 * @param signature Buffer to receive signature.
 * @param signature_len Pointer to buffer size (in) / actual size (out).
 * @return AVP_OK on success.
 */
avp_ret_t avp_hw_sign(avp_vault_t *vault, const char *key_name,
                      const uint8_t *data, size_t data_len,
                      uint8_t *signature, size_t *signature_len);

/**
 * @brief HW_ATTEST operation - prove secret is stored in hardware.
 *
 * @param vault Pointer to vault handle.
 * @param name Secret name to attest.
 * @param attestation Pointer to attestation result.
 * @return AVP_OK on success.
 */
avp_ret_t avp_hw_attest(avp_vault_t *vault, const char *name, avp_attestation_t *attestation);

/*=============================================================================
 * Utility Functions
 *============================================================================*/

/**
 * @brief Check if session is active.
 *
 * @param vault Pointer to vault handle.
 * @return true if session is active and not expired.
 */
bool avp_session_active(const avp_vault_t *vault);

/**
 * @brief Get error message for AVP error code.
 *
 * @param ret Error code.
 * @return Human-readable error message.
 */
const char *avp_strerror(avp_ret_t ret);

#ifdef __cplusplus
}
#endif

#endif /* AVP_TROPIC_H */
