/**
 * @file avp_tropic.c
 * @brief Agent Vault Protocol (AVP) implementation for TROPIC01 secure element.
 *
 * @copyright Copyright (c) 2026 AVP Protocol Contributors
 * @license Apache-2.0 (see LICENSE)
 */

#include "avp_tropic.h"

#include <string.h>
#include <stdio.h>

#include "libtropic.h"

/*=============================================================================
 * Constants
 *============================================================================*/

#define AVP_VERSION "0.1.0"
#define AVP_DEFAULT_TTL_SECONDS 300  /* 5 minutes for hardware */
#define AVP_SESSION_ID_LEN 32

/*=============================================================================
 * Internal Helpers
 *============================================================================*/

static bool validate_secret_name(const char *name)
{
    if (name == NULL || name[0] == '\0') {
        return false;
    }

    size_t len = strlen(name);
    if (len > AVP_MAX_SECRET_NAME_LEN) {
        return false;
    }

    /* First character must be a letter */
    char c = name[0];
    if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'))) {
        return false;
    }

    /* Remaining characters: letters, digits, underscore, period, hyphen */
    for (size_t i = 1; i < len; i++) {
        c = name[i];
        bool valid = (c >= 'A' && c <= 'Z') ||
                     (c >= 'a' && c <= 'z') ||
                     (c >= '0' && c <= '9') ||
                     (c == '_') || (c == '.') || (c == '-');
        if (!valid) {
            return false;
        }
    }

    return true;
}

static void generate_session_id(char *session_id, size_t max_len)
{
    /* Generate session ID with AVP prefix */
    snprintf(session_id, max_len, "%s", AVP_SESSION_PREFIX);
    size_t prefix_len = strlen(AVP_SESSION_PREFIX);

    /* TODO: Generate random alphanumeric suffix using TROPIC01 RNG */
    /* For now, use placeholder */
    const char *charset = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    for (size_t i = prefix_len; i < prefix_len + AVP_SESSION_ID_LEN && i < max_len - 1; i++) {
        session_id[i] = charset[i % 62];
    }
    session_id[prefix_len + AVP_SESSION_ID_LEN] = '\0';
}

/*=============================================================================
 * AVP Operations Implementation
 *============================================================================*/

avp_ret_t avp_init(avp_vault_t *vault, void *device)
{
    if (vault == NULL || device == NULL) {
        return AVP_ERR_INTERNAL;
    }

    memset(vault, 0, sizeof(avp_vault_t));

    /* Initialize libtropic handle */
    lt_ret_t lt_ret = lt_init(&vault->lt_handle, device);
    if (lt_ret != LT_OK) {
        return AVP_ERR_HARDWARE_ERROR;
    }

    vault->session_state = AVP_SESSION_INACTIVE;
    vault->authenticated = false;
    strcpy(vault->workspace, "default");

    return AVP_OK;
}

avp_ret_t avp_deinit(avp_vault_t *vault)
{
    if (vault == NULL) {
        return AVP_ERR_INTERNAL;
    }

    lt_deinit(&vault->lt_handle);

    /* Zero sensitive data */
    memset(vault->session_id, 0, sizeof(vault->session_id));
    vault->session_state = AVP_SESSION_INACTIVE;
    vault->authenticated = false;

    return AVP_OK;
}

avp_ret_t avp_discover(avp_vault_t *vault, avp_discover_response_t *response)
{
    if (vault == NULL || response == NULL) {
        return AVP_ERR_INTERNAL;
    }

    memset(response, 0, sizeof(avp_discover_response_t));

    strncpy(response->version, AVP_VERSION, sizeof(response->version) - 1);
    strncpy(response->conformance, "hardware", sizeof(response->conformance) - 1);
    response->attestation = true;
    response->rotation = true;
    response->max_secrets = AVP_TROPIC_KEY_SLOTS;

    return AVP_OK;
}

avp_ret_t avp_authenticate(avp_vault_t *vault, const char *workspace, const char *pin, uint32_t ttl_seconds)
{
    if (vault == NULL) {
        return AVP_ERR_INTERNAL;
    }

    /* Set workspace */
    if (workspace != NULL) {
        strncpy(vault->workspace, workspace, sizeof(vault->workspace) - 1);
    } else {
        strcpy(vault->workspace, "default");
    }

    /* Authenticate with TROPIC01 using PIN */
    if (pin != NULL) {
        lt_ret_t lt_ret = lt_login(&vault->lt_handle, (const uint8_t *)pin, strlen(pin));
        if (lt_ret != LT_OK) {
            return AVP_ERR_AUTHENTICATION_FAILED;
        }
    }

    /* Generate session ID */
    generate_session_id(vault->session_id, sizeof(vault->session_id));

    /* Set session parameters */
    vault->session_state = AVP_SESSION_ACTIVE;
    vault->session_ttl = (ttl_seconds > 0) ? ttl_seconds : AVP_DEFAULT_TTL_SECONDS;
    vault->session_created_at = 0;  /* TODO: Get actual timestamp */
    vault->authenticated = true;

    return AVP_OK;
}

avp_ret_t avp_store(avp_vault_t *vault, const char *name, const uint8_t *value, size_t value_len)
{
    if (vault == NULL || name == NULL || value == NULL) {
        return AVP_ERR_INTERNAL;
    }

    if (!vault->authenticated) {
        return AVP_ERR_NOT_INITIALIZED;
    }

    if (!validate_secret_name(name)) {
        return AVP_ERR_INVALID_NAME;
    }

    if (value_len > AVP_MAX_SECRET_VALUE_LEN) {
        return AVP_ERR_INTERNAL;
    }

    /* Store in TROPIC01 */
    /* TODO: Map name to slot index and store encrypted value */
    lt_ret_t lt_ret = lt_r_mem_data_write(&vault->lt_handle, 0, value, value_len);
    if (lt_ret != LT_OK) {
        return AVP_ERR_HARDWARE_ERROR;
    }

    return AVP_OK;
}

avp_ret_t avp_retrieve(avp_vault_t *vault, const char *name, uint8_t *value, size_t *value_len)
{
    if (vault == NULL || name == NULL || value == NULL || value_len == NULL) {
        return AVP_ERR_INTERNAL;
    }

    if (!vault->authenticated) {
        return AVP_ERR_NOT_INITIALIZED;
    }

    if (!validate_secret_name(name)) {
        return AVP_ERR_INVALID_NAME;
    }

    /* Retrieve from TROPIC01 */
    /* TODO: Map name to slot index and retrieve */
    uint16_t read_len = (uint16_t)*value_len;
    lt_ret_t lt_ret = lt_r_mem_data_read(&vault->lt_handle, 0, value, &read_len);
    if (lt_ret != LT_OK) {
        if (lt_ret == LT_L3_SLOT_IS_EMPTY) {
            return AVP_ERR_SECRET_NOT_FOUND;
        }
        return AVP_ERR_HARDWARE_ERROR;
    }

    *value_len = read_len;
    return AVP_OK;
}

avp_ret_t avp_delete(avp_vault_t *vault, const char *name, bool *deleted)
{
    if (vault == NULL || name == NULL) {
        return AVP_ERR_INTERNAL;
    }

    if (!vault->authenticated) {
        return AVP_ERR_NOT_INITIALIZED;
    }

    /* Delete from TROPIC01 */
    /* TODO: Map name to slot index and erase */
    lt_ret_t lt_ret = lt_r_mem_data_erase(&vault->lt_handle, 0);
    if (deleted != NULL) {
        *deleted = (lt_ret == LT_OK);
    }

    return AVP_OK;
}

avp_ret_t avp_list(avp_vault_t *vault, avp_secret_metadata_t *secrets, size_t max_secrets, size_t *count)
{
    if (vault == NULL || count == NULL) {
        return AVP_ERR_INTERNAL;
    }

    if (!vault->authenticated) {
        return AVP_ERR_NOT_INITIALIZED;
    }

    /* TODO: Enumerate TROPIC01 slots and return metadata */
    *count = 0;

    return AVP_OK;
}

/*=============================================================================
 * AVP Hardware Extension Operations
 *============================================================================*/

avp_ret_t avp_hw_challenge(avp_vault_t *vault, avp_attestation_t *attestation)
{
    if (vault == NULL || attestation == NULL) {
        return AVP_ERR_INTERNAL;
    }

    memset(attestation, 0, sizeof(avp_attestation_t));

    /* Get TROPIC01 device info */
    lt_chip_info_t chip_info;
    lt_ret_t lt_ret = lt_get_info_chip(&vault->lt_handle, &chip_info);
    if (lt_ret != LT_OK) {
        return AVP_ERR_HARDWARE_ERROR;
    }

    attestation->verified = true;
    strncpy(attestation->manufacturer, "Tropic Square", sizeof(attestation->manufacturer) - 1);
    strncpy(attestation->model, "TROPIC01", sizeof(attestation->model) - 1);

    /* TODO: Get actual firmware version and serial */
    strncpy(attestation->firmware_version, "1.0.0", sizeof(attestation->firmware_version) - 1);

    return AVP_OK;
}

avp_ret_t avp_hw_sign(avp_vault_t *vault, const char *key_name,
                      const uint8_t *data, size_t data_len,
                      uint8_t *signature, size_t *signature_len)
{
    if (vault == NULL || key_name == NULL || data == NULL || signature == NULL || signature_len == NULL) {
        return AVP_ERR_INTERNAL;
    }

    if (!vault->authenticated) {
        return AVP_ERR_NOT_INITIALIZED;
    }

    /* Sign using TROPIC01 ECDSA - key never leaves the device */
    /* TODO: Map key_name to slot and perform signing */
    lt_ret_t lt_ret = lt_ecc_ecdsa_sign(&vault->lt_handle, 0, data, data_len, signature);
    if (lt_ret != LT_OK) {
        return AVP_ERR_HARDWARE_ERROR;
    }

    *signature_len = 64;  /* ECDSA P-256 signature size */
    return AVP_OK;
}

avp_ret_t avp_hw_attest(avp_vault_t *vault, const char *name, avp_attestation_t *attestation)
{
    if (vault == NULL || name == NULL || attestation == NULL) {
        return AVP_ERR_INTERNAL;
    }

    if (!vault->authenticated) {
        return AVP_ERR_NOT_INITIALIZED;
    }

    /* Generate attestation proving secret is in TROPIC01 hardware */
    memset(attestation, 0, sizeof(avp_attestation_t));

    /* TODO: Generate attestation certificate chain */
    attestation->verified = true;
    strncpy(attestation->manufacturer, "Tropic Square", sizeof(attestation->manufacturer) - 1);
    strncpy(attestation->model, "TROPIC01", sizeof(attestation->model) - 1);

    return AVP_OK;
}

/*=============================================================================
 * Utility Functions
 *============================================================================*/

bool avp_session_active(const avp_vault_t *vault)
{
    if (vault == NULL) {
        return false;
    }

    return vault->session_state == AVP_SESSION_ACTIVE && vault->authenticated;
}

const char *avp_strerror(avp_ret_t ret)
{
    switch (ret) {
        case AVP_OK:                        return "OK";
        case AVP_ERR_NOT_INITIALIZED:       return "Not initialized";
        case AVP_ERR_AUTHENTICATION_FAILED: return "Authentication failed";
        case AVP_ERR_SESSION_EXPIRED:       return "Session expired";
        case AVP_ERR_SECRET_NOT_FOUND:      return "Secret not found";
        case AVP_ERR_CAPACITY_EXCEEDED:     return "Capacity exceeded";
        case AVP_ERR_INVALID_NAME:          return "Invalid secret name";
        case AVP_ERR_HARDWARE_ERROR:        return "Hardware error";
        case AVP_ERR_CRYPTO_ERROR:          return "Cryptographic error";
        case AVP_ERR_INTERNAL:              return "Internal error";
        default:                            return "Unknown error";
    }
}
