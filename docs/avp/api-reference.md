# AVP-Tropic API Reference

## Overview

The AVP-Tropic API provides a C interface for the Agent Vault Protocol Hardware conformance level. All functions are thread-safe when used with separate `avp_vault_t` instances.

## Header

```c
#include "avp_tropic.h"
```

## Types

### avp_ret_t

Return codes for all AVP operations.

```c
typedef enum {
    AVP_OK = 0,                      // Success
    AVP_ERR_NOT_INITIALIZED,         // Vault not initialized or session expired
    AVP_ERR_AUTHENTICATION_FAILED,   // PIN incorrect or auth failure
    AVP_ERR_SESSION_EXPIRED,         // Session TTL exceeded
    AVP_ERR_SECRET_NOT_FOUND,        // Secret does not exist
    AVP_ERR_CAPACITY_EXCEEDED,       // No more slots available
    AVP_ERR_INVALID_NAME,            // Secret name doesn't match spec
    AVP_ERR_HARDWARE_ERROR,          // TROPIC01 communication error
    AVP_ERR_CRYPTO_ERROR,            // Cryptographic operation failed
    AVP_ERR_INTERNAL,                // Internal error (NULL pointer, etc.)
} avp_ret_t;
```

### avp_vault_t

Opaque vault handle. Must be initialized with `avp_init()`.

```c
typedef struct avp_vault_t {
    lt_handle_t lt_handle;           // libtropic handle
    avp_session_state_t session_state;
    char session_id[64];
    uint32_t session_created_at;
    uint32_t session_ttl;
    char workspace[256];
    bool authenticated;
} avp_vault_t;
```

### avp_session_state_t

Session lifecycle states.

```c
typedef enum {
    AVP_SESSION_INACTIVE = 0,        // Not authenticated
    AVP_SESSION_ACTIVE,              // Authenticated, operations allowed
    AVP_SESSION_EXPIRED,             // TTL exceeded
    AVP_SESSION_TERMINATED,          // Explicitly terminated
} avp_session_state_t;
```

### avp_discover_response_t

Response from DISCOVER operation.

```c
typedef struct avp_discover_response_t {
    char version[16];                // AVP protocol version ("0.1.0")
    char conformance[16];            // Conformance level ("hardware")
    bool attestation;                // Supports HW_ATTEST
    bool rotation;                   // Supports ROTATE
    uint16_t max_secrets;            // Maximum storable secrets
} avp_discover_response_t;
```

### avp_secret_metadata_t

Secret metadata returned by LIST operation.

```c
typedef struct avp_secret_metadata_t {
    char name[AVP_MAX_SECRET_NAME_LEN + 1];  // Secret name
    uint32_t created_at;                      // Unix timestamp
    uint32_t updated_at;                      // Unix timestamp
    uint8_t slot_index;                       // TROPIC01 slot
    uint32_t version;                         // Version counter
} avp_secret_metadata_t;
```

### avp_attestation_t

Hardware attestation result.

```c
typedef struct avp_attestation_t {
    bool verified;                   // Device verified authentic
    char manufacturer[64];           // "Tropic Square"
    char model[64];                  // "TROPIC01"
    char firmware_version[32];       // Firmware version string
    char serial[64];                 // Device serial number
    uint8_t certificate[2048];       // X.509 attestation certificate
    size_t certificate_len;          // Certificate length
} avp_attestation_t;
```

## Constants

```c
#define AVP_MAX_SECRET_NAME_LEN   255      // Max secret name length
#define AVP_MAX_SECRET_VALUE_LEN  65536    // Max secret value (64KB)
#define AVP_TROPIC_KEY_SLOTS      128      // TROPIC01 slot count
#define AVP_SESSION_PREFIX        "avp_sess_"  // Session ID prefix
```

---

## Core Functions

### avp_init

Initialize the AVP vault with a TROPIC01 backend.

```c
avp_ret_t avp_init(avp_vault_t *vault, void *device);
```

**Parameters:**
| Parameter | Type | Description |
|-----------|------|-------------|
| `vault` | `avp_vault_t *` | Pointer to vault handle to initialize |
| `device` | `void *` | Platform-specific device structure |

**Returns:** `AVP_OK` on success, `AVP_ERR_INTERNAL` on NULL parameters, `AVP_ERR_HARDWARE_ERROR` on communication failure.

**Example:**
```c
lt_dev_stm32u5_tropic_click_t device = {
    .spi_instance = SPI1,
    .spi_cs_gpio_pin = GPIO_PIN_14,
    .spi_cs_gpio_port = GPIOD,
    .rng_handle = &hrng,
};

avp_vault_t vault;
avp_ret_t ret = avp_init(&vault, &device);
if (ret != AVP_OK) {
    printf("Init failed: %s\n", avp_strerror(ret));
}
```

---

### avp_deinit

Deinitialize the vault and clean up resources.

```c
avp_ret_t avp_deinit(avp_vault_t *vault);
```

**Parameters:**
| Parameter | Type | Description |
|-----------|------|-------------|
| `vault` | `avp_vault_t *` | Pointer to vault handle |

**Returns:** `AVP_OK` on success.

**Notes:**
- Zeroes sensitive data (session ID, etc.)
- Releases SPI and GPIO resources
- Safe to call multiple times

---

### avp_discover

Query vault capabilities (DISCOVER operation).

```c
avp_ret_t avp_discover(avp_vault_t *vault, avp_discover_response_t *response);
```

**Parameters:**
| Parameter | Type | Description |
|-----------|------|-------------|
| `vault` | `avp_vault_t *` | Pointer to vault handle |
| `response` | `avp_discover_response_t *` | Output response structure |

**Returns:** `AVP_OK` on success.

**Example:**
```c
avp_discover_response_t info;
avp_discover(&vault, &info);

printf("AVP Version: %s\n", info.version);
printf("Conformance: %s\n", info.conformance);
printf("Max secrets: %d\n", info.max_secrets);
printf("Attestation: %s\n", info.attestation ? "yes" : "no");
```

**Output:**
```
AVP Version: 0.1.0
Conformance: hardware
Max secrets: 128
Attestation: yes
```

---

### avp_authenticate

Establish an authenticated session (AUTHENTICATE operation).

```c
avp_ret_t avp_authenticate(
    avp_vault_t *vault,
    const char *workspace,
    const char *pin,
    uint32_t ttl_seconds
);
```

**Parameters:**
| Parameter | Type | Description |
|-----------|------|-------------|
| `vault` | `avp_vault_t *` | Pointer to vault handle |
| `workspace` | `const char *` | Workspace name (NULL for "default") |
| `pin` | `const char *` | PIN for hardware authentication |
| `ttl_seconds` | `uint32_t` | Session TTL (0 for default 300s) |

**Returns:**
- `AVP_OK` — Authentication successful
- `AVP_ERR_AUTHENTICATION_FAILED` — Wrong PIN or device locked
- `AVP_ERR_HARDWARE_ERROR` — Communication failure

**Example:**
```c
// Authenticate with PIN
avp_ret_t ret = avp_authenticate(&vault, "production", "123456", 600);
if (ret == AVP_ERR_AUTHENTICATION_FAILED) {
    printf("Wrong PIN!\n");
    return;
}

// Session is now active for 600 seconds
printf("Session ID: %s\n", vault.session_id);
```

**Security Notes:**
- PIN is transmitted to TROPIC01 over encrypted L2 channel
- PIN is zeroed from host memory after transmission
- Failed attempts trigger exponential backoff
- After N failures, device enters lockout

---

### avp_store

Store a secret in the vault (STORE operation).

```c
avp_ret_t avp_store(
    avp_vault_t *vault,
    const char *name,
    const uint8_t *value,
    size_t value_len
);
```

**Parameters:**
| Parameter | Type | Description |
|-----------|------|-------------|
| `vault` | `avp_vault_t *` | Pointer to vault handle |
| `name` | `const char *` | Secret name (1-255 chars) |
| `value` | `const uint8_t *` | Secret value |
| `value_len` | `size_t` | Value length in bytes |

**Returns:**
- `AVP_OK` — Secret stored
- `AVP_ERR_NOT_INITIALIZED` — Not authenticated
- `AVP_ERR_INVALID_NAME` — Name doesn't match spec
- `AVP_ERR_CAPACITY_EXCEEDED` — No available slots
- `AVP_ERR_HARDWARE_ERROR` — Storage failure

**Name Requirements:**
- Length: 1-255 characters
- First character: `[A-Za-z]`
- Subsequent: `[A-Za-z0-9_.-]`
- Case-sensitive

**Example:**
```c
const char *api_key = "sk-ant-api03-...";
avp_ret_t ret = avp_store(
    &vault,
    "anthropic_api_key",
    (const uint8_t *)api_key,
    strlen(api_key)
);

if (ret == AVP_OK) {
    printf("Secret stored in TROPIC01\n");
}
```

---

### avp_retrieve

Retrieve a secret from the vault (RETRIEVE operation).

```c
avp_ret_t avp_retrieve(
    avp_vault_t *vault,
    const char *name,
    uint8_t *value,
    size_t *value_len
);
```

**Parameters:**
| Parameter | Type | Description |
|-----------|------|-------------|
| `vault` | `avp_vault_t *` | Pointer to vault handle |
| `name` | `const char *` | Secret name |
| `value` | `uint8_t *` | Output buffer for value |
| `value_len` | `size_t *` | In: buffer size, Out: actual size |

**Returns:**
- `AVP_OK` — Secret retrieved
- `AVP_ERR_NOT_INITIALIZED` — Not authenticated
- `AVP_ERR_SECRET_NOT_FOUND` — Secret doesn't exist
- `AVP_ERR_HARDWARE_ERROR` — Retrieval failure

**Example:**
```c
uint8_t value[256];
size_t value_len = sizeof(value);

avp_ret_t ret = avp_retrieve(&vault, "anthropic_api_key", value, &value_len);
if (ret == AVP_OK) {
    // Use the API key
    value[value_len] = '\0';  // Null-terminate if string
    call_anthropic_api((char *)value);

    // Zero sensitive data when done
    memset(value, 0, sizeof(value));
}
```

---

### avp_delete

Delete a secret from the vault (DELETE operation).

```c
avp_ret_t avp_delete(
    avp_vault_t *vault,
    const char *name,
    bool *deleted
);
```

**Parameters:**
| Parameter | Type | Description |
|-----------|------|-------------|
| `vault` | `avp_vault_t *` | Pointer to vault handle |
| `name` | `const char *` | Secret name |
| `deleted` | `bool *` | Output: true if secret existed |

**Returns:** `AVP_OK` always (idempotent).

**Example:**
```c
bool was_deleted;
avp_delete(&vault, "old_api_key", &was_deleted);

if (was_deleted) {
    printf("Secret securely erased\n");
} else {
    printf("Secret did not exist\n");
}
```

---

### avp_list

Enumerate secrets in the workspace (LIST operation).

```c
avp_ret_t avp_list(
    avp_vault_t *vault,
    avp_secret_metadata_t *secrets,
    size_t max_secrets,
    size_t *count
);
```

**Parameters:**
| Parameter | Type | Description |
|-----------|------|-------------|
| `vault` | `avp_vault_t *` | Pointer to vault handle |
| `secrets` | `avp_secret_metadata_t *` | Output array |
| `max_secrets` | `size_t` | Maximum entries to return |
| `count` | `size_t *` | Output: actual count |

**Returns:** `AVP_OK` on success.

**Example:**
```c
avp_secret_metadata_t secrets[32];
size_t count;

avp_list(&vault, secrets, 32, &count);

printf("Found %zu secrets:\n", count);
for (size_t i = 0; i < count; i++) {
    printf("  - %s (slot %d, v%d)\n",
           secrets[i].name,
           secrets[i].slot_index,
           secrets[i].version);
}
```

---

## Hardware Extension Functions

### avp_hw_challenge

Verify device authenticity (HW_CHALLENGE operation).

```c
avp_ret_t avp_hw_challenge(
    avp_vault_t *vault,
    avp_attestation_t *attestation
);
```

**Parameters:**
| Parameter | Type | Description |
|-----------|------|-------------|
| `vault` | `avp_vault_t *` | Pointer to vault handle |
| `attestation` | `avp_attestation_t *` | Output attestation data |

**Returns:** `AVP_OK` on success, `AVP_ERR_HARDWARE_ERROR` on failure.

**Example:**
```c
avp_attestation_t att;
avp_hw_challenge(&vault, &att);

if (att.verified) {
    printf("Device verified: %s %s\n", att.manufacturer, att.model);
    printf("Firmware: %s\n", att.firmware_version);
    printf("Serial: %s\n", att.serial);
}
```

---

### avp_hw_sign

Sign data with a hardware key (HW_SIGN operation).

The signing key **never leaves** the TROPIC01 secure element.

```c
avp_ret_t avp_hw_sign(
    avp_vault_t *vault,
    const char *key_name,
    const uint8_t *data,
    size_t data_len,
    uint8_t *signature,
    size_t *signature_len
);
```

**Parameters:**
| Parameter | Type | Description |
|-----------|------|-------------|
| `vault` | `avp_vault_t *` | Pointer to vault handle |
| `key_name` | `const char *` | Name of signing key |
| `data` | `const uint8_t *` | Data to sign |
| `data_len` | `size_t` | Data length |
| `signature` | `uint8_t *` | Output signature buffer (64+ bytes) |
| `signature_len` | `size_t *` | In: buffer size, Out: signature size |

**Returns:**
- `AVP_OK` — Signature generated
- `AVP_ERR_SECRET_NOT_FOUND` — Key doesn't exist
- `AVP_ERR_HARDWARE_ERROR` — Signing failure

**Example:**
```c
uint8_t data[] = "message to sign";
uint8_t signature[64];
size_t sig_len = sizeof(signature);

avp_ret_t ret = avp_hw_sign(
    &vault,
    "signing_key",
    data, sizeof(data),
    signature, &sig_len
);

if (ret == AVP_OK) {
    printf("Signature (%zu bytes): ", sig_len);
    for (size_t i = 0; i < sig_len; i++) {
        printf("%02x", signature[i]);
    }
    printf("\n");
}
```

---

### avp_hw_attest

Generate attestation proof for a secret (HW_ATTEST operation).

```c
avp_ret_t avp_hw_attest(
    avp_vault_t *vault,
    const char *name,
    avp_attestation_t *attestation
);
```

**Parameters:**
| Parameter | Type | Description |
|-----------|------|-------------|
| `vault` | `avp_vault_t *` | Pointer to vault handle |
| `name` | `const char *` | Secret name to attest |
| `attestation` | `avp_attestation_t *` | Output attestation |

**Returns:** `AVP_OK` on success.

**Example:**
```c
avp_attestation_t att;
avp_hw_attest(&vault, "anthropic_api_key", &att);

// att.certificate contains X.509 certificate chain proving
// the secret is stored in FIPS 140-3 Level 3 hardware
printf("Certificate length: %zu bytes\n", att.certificate_len);
```

---

## Utility Functions

### avp_session_active

Check if session is active and not expired.

```c
bool avp_session_active(const avp_vault_t *vault);
```

**Example:**
```c
if (!avp_session_active(&vault)) {
    avp_authenticate(&vault, NULL, pin, 0);
}
```

---

### avp_strerror

Get human-readable error message.

```c
const char *avp_strerror(avp_ret_t ret);
```

**Example:**
```c
avp_ret_t ret = avp_store(&vault, "key", value, len);
if (ret != AVP_OK) {
    printf("Error: %s\n", avp_strerror(ret));
}
```

---

## Complete Example

```c
#include "avp_tropic.h"
#include "libtropic_port_stm32u5_tropic_click.h"

int main(void) {
    /* Initialize HAL */
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_SPI1_Init();
    MX_RNG_Init();

    /* Configure device */
    lt_dev_stm32u5_tropic_click_t device = {
        .spi_instance = SPI1,
        .baudrate_prescaler = SPI_BAUDRATEPRESCALER_16,
        .spi_cs_gpio_pin = GPIO_PIN_14,
        .spi_cs_gpio_port = GPIOD,
        .int_gpio_pin = GPIO_PIN_13,
        .int_gpio_port = GPIOF,
        .rng_handle = &hrng,
    };

    /* Initialize AVP vault */
    avp_vault_t vault;
    avp_ret_t ret = avp_init(&vault, &device);
    if (ret != AVP_OK) {
        printf("Init failed: %s\n", avp_strerror(ret));
        return 1;
    }

    /* Discover capabilities */
    avp_discover_response_t info;
    avp_discover(&vault, &info);
    printf("AVP %s (%s conformance)\n", info.version, info.conformance);

    /* Authenticate */
    ret = avp_authenticate(&vault, "production", "123456", 300);
    if (ret != AVP_OK) {
        printf("Auth failed: %s\n", avp_strerror(ret));
        return 1;
    }
    printf("Session: %s\n", vault.session_id);

    /* Store a secret */
    const char *api_key = "sk-ant-api03-xxx";
    ret = avp_store(&vault, "anthropic_api_key",
                    (uint8_t *)api_key, strlen(api_key));
    printf("Store: %s\n", avp_strerror(ret));

    /* Retrieve the secret */
    uint8_t value[256];
    size_t value_len = sizeof(value);
    ret = avp_retrieve(&vault, "anthropic_api_key", value, &value_len);
    if (ret == AVP_OK) {
        printf("Retrieved %zu bytes\n", value_len);
    }

    /* Hardware attestation */
    avp_attestation_t att;
    avp_hw_challenge(&vault, &att);
    printf("Device: %s %s (verified: %s)\n",
           att.manufacturer, att.model,
           att.verified ? "yes" : "no");

    /* Clean up */
    memset(value, 0, sizeof(value));
    avp_deinit(&vault);

    return 0;
}
```
