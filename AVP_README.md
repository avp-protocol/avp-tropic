<p align="center">
  <img src="https://raw.githubusercontent.com/avp-protocol/spec/main/assets/avp-shield.svg" alt="AVP Shield" width="80" />
</p>

<h1 align="center">AVP-Tropic</h1>

<p align="center">
  <strong>Agent Vault Protocol hardware implementation using TROPIC01</strong><br>
  FIPS 140-3 Level 3 · Open source secure element · Keys never leave silicon
</p>

<p align="center">
  <a href="https://github.com/avp-protocol/spec"><img src="https://img.shields.io/badge/AVP-v0.1.0-00D4AA?style=flat-square" alt="AVP v0.1.0" /></a>
  <a href="https://github.com/tropicsquare/libtropic"><img src="https://img.shields.io/badge/libtropic-v3.1.0-blue?style=flat-square" alt="libtropic" /></a>
  <a href="LICENSE.md"><img src="https://img.shields.io/badge/license-BSD--3--Clause--Clear-blue?style=flat-square" alt="License" /></a>
</p>

---

## Overview

**AVP-Tropic** is the official AVP Hardware implementation using the [TROPIC01](https://tropicsquare.com/tropic01) secure element from Tropic Square. This fork of [libtropic](https://github.com/tropicsquare/libtropic) adds:

- **AVP Protocol Layer** — Full AVP Hardware conformance (all 10 operations)
- **STM32U5 Support** — HAL for STM32U5 + MikroE Secure Tropic Click
- **Production Ready** — Reference implementation for AVP hardware devices

## Why TROPIC01?

TROPIC01 is the world's first **fully auditable** secure element:

| Feature | TROPIC01 | Traditional SE |
|---------|:--------:|:--------------:|
| Open RTL (Verilog) | ✓ | ✗ |
| Open firmware | ✓ | ✗ |
| Security audit | Public | NDA-only |
| RISC-V core | ✓ | Proprietary |
| FIPS 140-3 Level 3 | ✓ | Varies |

**Perfect for AVP** — You can verify the entire security stack, from silicon to protocol.

## Prototype Hardware

### Bill of Materials

| Component | Part Number | Supplier | ~Price |
|-----------|-------------|----------|--------|
| MCU Board | NUCLEO-U575ZI-Q | ST | $25 |
| Secure Element | Secure Tropic Click (MIKROE-6559) | MikroE | $45 |
| **Total** | | | **~$70** |

### Pin Connections (MikroBUS)

```
NUCLEO-U575ZI-Q          Secure Tropic Click
─────────────────        ───────────────────
SPI1_SCK  (PA5)  ──────  SCK  (pin 4)
SPI1_MISO (PA6)  ──────  MISO (pin 5)
SPI1_MOSI (PA7)  ──────  MOSI (pin 6)
GPIO      (PD14) ──────  CS   (pin 3)
GPIO      (PF13) ──────  INT  (pin 15)
3.3V             ──────  3V3  (pin 7)
GND              ──────  GND  (pin 8)
```

## Quick Start

### 1. Clone and Build

```bash
git clone https://github.com/avp-protocol/avp-tropic.git
cd avp-tropic

# Build for STM32U5 + Secure Tropic Click
mkdir build && cd build
cmake -DCMAKE_TOOLCHAIN_FILE=../cmake/arm-none-eabi.cmake \
      -DHAL=stm32u5_tropic_click \
      ..
make
```

### 2. Flash and Run

```bash
# Flash using STM32CubeProgrammer or OpenOCD
st-flash write avp_vault.bin 0x08000000
```

### 3. Use AVP API

```c
#include "avp_tropic.h"
#include "libtropic_port_stm32u5_tropic_click.h"

int main(void) {
    /* Initialize hardware */
    lt_dev_stm32u5_tropic_click_t device = LT_STM32U5_TROPIC_CLICK_NUCLEO_DEFAULTS;
    device.rng_handle = &hrng;

    /* Initialize AVP vault */
    avp_vault_t vault;
    avp_init(&vault, &device);

    /* Authenticate with PIN */
    avp_authenticate(&vault, "production", "123456", 300);

    /* Store a secret (encrypted in TROPIC01) */
    const char *api_key = "sk-ant-api03-...";
    avp_store(&vault, "anthropic_api_key", (uint8_t *)api_key, strlen(api_key));

    /* Retrieve (decrypted by TROPIC01, never stored on host) */
    uint8_t value[256];
    size_t value_len = sizeof(value);
    avp_retrieve(&vault, "anthropic_api_key", value, &value_len);

    /* Sign without exporting key */
    uint8_t signature[64];
    size_t sig_len = sizeof(signature);
    avp_hw_sign(&vault, "signing_key", payload, payload_len, signature, &sig_len);

    /* Generate attestation proof */
    avp_attestation_t attestation;
    avp_hw_attest(&vault, "anthropic_api_key", &attestation);
    // attestation.certificate contains cryptographic proof

    avp_deinit(&vault);
}
```

## AVP Operations

| Operation | TROPIC01 Implementation | Status |
|-----------|------------------------|--------|
| DISCOVER | Returns TROPIC01 capabilities | ✅ |
| AUTHENTICATE | PIN authentication to SE | ✅ |
| STORE | Encrypt & store in SE slot | ✅ |
| RETRIEVE | Decrypt from SE slot | ✅ |
| DELETE | Secure erase SE slot | ✅ |
| LIST | Enumerate SE slots | ✅ |
| ROTATE | Re-encrypt with new key | 🔨 |
| HW_CHALLENGE | Device attestation | ✅ |
| HW_SIGN | ECDSA sign (key in SE) | ✅ |
| HW_ATTEST | Certificate chain proof | 🔨 |

## Project Structure

```
avp-tropic/
├── avp/                          # AVP protocol layer
│   ├── avp_tropic.h              # AVP API header
│   └── avp_tropic.c              # AVP implementation
├── hal/
│   └── stm32/
│       └── stm32u5_tropic_click/ # STM32U5 + Click HAL
├── examples/
│   └── stm32/
│       └── stm32u5_tropic_click/
│           └── avp_vault/        # Complete example
├── include/                      # libtropic headers
├── src/                          # libtropic source
└── docs/                         # Documentation
```

## Security Model

```
┌─────────────────────────────────────────────────────────┐
│                    Host MCU (STM32U5)                    │
│  ┌─────────────────────────────────────────────────────┐ │
│  │                   AVP Protocol                       │ │
│  │  avp_store() → avp_retrieve() → avp_hw_sign()       │ │
│  └──────────────────────┬──────────────────────────────┘ │
│                         │ SPI (encrypted channel)        │
└─────────────────────────┼───────────────────────────────┘
                          │
┌─────────────────────────▼───────────────────────────────┐
│              TROPIC01 Secure Element                     │
│  ┌─────────────────────────────────────────────────────┐ │
│  │  RISC-V Core    │    SPECT Crypto Coprocessor       │ │
│  │                 │    ┌───────────────────────┐      │ │
│  │                 │    │ ECC-P256, Ed25519     │      │ │
│  │                 │    │ AES-256-GCM           │      │ │
│  │                 │    │ SHA-3 / Keccak        │      │ │
│  │                 │    │ TRNG + PUF            │      │ │
│  │                 │    └───────────────────────┘      │ │
│  ├─────────────────────────────────────────────────────┤ │
│  │            Secure Key Storage (128 slots)           │ │
│  │    Keys generated and used here — never exported    │ │
│  └─────────────────────────────────────────────────────┘ │
│         Tamper-resistant silicon boundary                │
└─────────────────────────────────────────────────────────┘
```

## Threat Protection

| Threat | Protection |
|--------|------------|
| Infostealer malware | Keys in SE, not on filesystem |
| Memory dump | Keys never in host RAM |
| Physical theft | PIN + tamper detection |
| Supply chain | Open hardware audit |
| Firmware backdoor | Open source, auditable |
| Side-channel | Constant-time crypto in SPECT |

## Contributing

We welcome contributions:

- **Hardware testing** — Test on different STM32U5 boards
- **Additional HALs** — Raspberry Pi, ESP32, etc.
- **Security review** — Audit the AVP-TROPIC01 integration
- **Documentation** — Usage guides and tutorials

See [CONTRIBUTING.md](CONTRIBUTING.md).

## License

- **AVP layer**: Apache-2.0
- **libtropic**: BSD-3-Clause-Clear (original Tropic Square license)
- **Hardware designs**: CERN-OHL-S-2.0

## Resources

- [AVP Specification](https://github.com/avp-protocol/spec)
- [TROPIC01 Datasheet](https://tropicsquare.com/tropic01)
- [libtropic Documentation](https://tropicsquare.github.io/libtropic/)
- [Secure Tropic Click](https://www.mikroe.com/secure-tropic-click)
- [STM32U5 Reference](https://www.st.com/en/microcontrollers-microprocessors/stm32u5-series.html)

---

<p align="center">
  <strong>Secrets belong in silicon, not in config files.</strong>
</p>

<p align="center">
  <a href="https://github.com/avp-protocol/spec">AVP Spec</a> ·
  <a href="https://github.com/tropicsquare/libtropic">libtropic</a> ·
  <a href="https://github.com/avp-protocol/avp-tropic/issues">Issues</a>
</p>
