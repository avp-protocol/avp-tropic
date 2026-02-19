<p align="center">
  <img src="https://raw.githubusercontent.com/avp-protocol/spec/main/assets/avp-shield.svg" alt="AVP Shield" width="80" />
</p>

<h1 align="center">avp-tropic</h1>

<p align="center">
  <strong>TROPIC01 Hardware Abstraction Layer for AVP</strong><br>
  Secure element SDK for Agent Vault Protocol hardware implementations
</p>

<p align="center">
  <a href="https://github.com/avp-protocol/avp-tropic/actions"><img src="https://img.shields.io/github/actions/workflow/status/avp-protocol/avp-tropic/ci.yml?style=flat-square" alt="CI" /></a>
  <a href="LICENSE"><img src="https://img.shields.io/badge/license-Apache_2.0-blue?style=flat-square" alt="License" /></a>
  <a href="https://tropicsquare.com/tropic01"><img src="https://img.shields.io/badge/Secure%20Element-TROPIC01-00D4AA?style=flat-square" alt="TROPIC01" /></a>
</p>

---

## Overview

`avp-tropic` is the TROPIC01 Hardware Abstraction Layer (HAL) for the AVP ecosystem. It provides the low-level SDK for communicating with TROPIC01 secure elements, which power the hardware security backends in AVP implementations like [NexusClaw](https://github.com/avp-protocol/nexusclaw).

This library is based on [Tropic Square's libtropic](https://github.com/tropicsquare/libtropic), adapted and extended for AVP protocol integration.

## Role in the AVP Ecosystem

```
+------------------+     +------------------+     +------------------+
|   AI Agent       |     |   AVP Client     |     |   avp-tropic     |
|  (LangChain,     | --> |  (avp-py,        | --> |  (This library)  |
|   CrewAI, etc.)  |     |   avp-rs, etc.)  |     |                  |
+------------------+     +------------------+     +--------+---------+
                                                          |
                                                          | SPI
                                                          v
                                              +------------------+
                                              |   TROPIC01       |
                                              |   Secure Element |
                                              +------------------+
```

**avp-tropic** handles:
- SPI communication with TROPIC01 chips
- Secure session establishment (L2 encrypted channel)
- Key storage and retrieval commands
- Cryptographic operations (sign, verify, encrypt)
- Attestation and device authentication

## Related Projects

| Project | Description |
|---------|-------------|
| [NexusClaw](https://github.com/avp-protocol/nexusclaw) | Production USB hardware security key using avp-tropic |
| [avp-hardware](https://github.com/avp-protocol/avp-hardware) | Reference hardware designs and firmware |
| [AVP Specification](https://github.com/avp-protocol/spec) | Protocol specification for hardware extensions |

## Supported Hardware

### Development Boards

| Board | MCU | Status |
|-------|-----|--------|
| [Secure Tropic Click](https://www.mikroe.com/secure-tropic-click) | Various (click board) | Supported |
| [STM32U5 Discovery](https://www.st.com/en/evaluation-tools/b-u585i-iot02a.html) + Tropic Click | STM32U585 | Supported |
| Custom NexusClaw PCB | STM32U535 | In Development |

### Secure Element

| Element | Features | Status |
|---------|----------|--------|
| TROPIC01 | 128 slots, ECC P-256/Ed25519, AES-256-GCM, SHA-3, TRNG | Supported |

## Features

- **Hardware Abstraction Layer** — Platform-independent HAL for host MCUs
- **Crypto Abstraction Layer** — Pluggable cryptographic backends
- **Secure Sessions** — Encrypted L2 communication with TROPIC01
- **Multi-Platform** — STM32, nRF, ESP32, Linux, and more

## Compatibility

For the library to function correctly with TROPIC01, component versions must be compatible:

| avp-tropic | Application FW | SPECT FW | Bootloader FW | Status |
|:----------:|:--------------:|:--------:|:-------------:|:------:|
| 3.1.0      | 1.0.0-2.0.0    | 1.0.0    | 2.0.1         | Current |
| 3.0.0      | 1.0.0-2.0.0    | 1.0.0    | 2.0.1         | Supported |
| 2.0.1      | 1.0.0-1.0.1    | 1.0.0    | 2.0.1         | Supported |

> **Warning:** Using mismatched versions may result in unpredictable behavior. Always use compatible versions.

## Repository Structure

```
avp-tropic/
├── avp/                # AVP protocol layer (added for AVP integration)
├── cal/                # Crypto Abstraction Layer implementations
├── hal/                # Hardware Abstraction Layer implementations
│   └── stm32/          # STM32 HAL
├── include/            # Public API headers
├── src/                # Core library source
├── examples/           # Example projects
│   └── stm32/          # STM32 examples
├── tests/              # Functional tests
├── docs/               # Documentation
└── vendor/             # Third-party dependencies
```

## Getting Started

### Prerequisites

```bash
# ARM toolchain
apt install gcc-arm-none-eabi

# CMake
apt install cmake
```

### Building

```bash
mkdir build && cd build
cmake ..
make
```

### Example: STM32U5 with Tropic Click

```bash
cd examples/stm32/stm32u5_tropic_click
make
```

See the [examples README](examples/stm32/stm32u5_tropic_click/README.md) for detailed instructions.

## AVP Integration

To use avp-tropic in an AVP hardware implementation:

```c
#include "avp_tropic.h"

// Initialize TROPIC01 interface
lt_handle_t handle;
lt_init(&handle);

// Open secure session
lt_open_session(&handle, pairing_key);

// Store an AVP secret
lt_slot_write(&handle, slot_id, secret_data, secret_len);

// Retrieve an AVP secret
lt_slot_read(&handle, slot_id, buffer, &buffer_len);

// Sign data (key never leaves chip)
lt_ecc_sign(&handle, key_slot, hash, signature);
```

## Documentation

- [API Reference](https://tropicsquare.github.io/libtropic/latest/) — Original libtropic documentation
- [AVP Hardware Spec](https://github.com/avp-protocol/spec) — AVP hardware extension specification
- [TROPIC01 Datasheet](https://github.com/tropicsquare/tropic01) — Secure element documentation

## Contributing

We welcome contributions! Areas where help is needed:

- **HAL ports** — New microcontroller platforms
- **Testing** — Integration tests with different hardware
- **Documentation** — Tutorials and examples

See [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.

## License

This project is licensed under Apache 2.0. See [LICENSE](LICENSE.md) for details.

Original libtropic code is from [Tropic Square](https://tropicsquare.com), used under their open-source license.

---

<p align="center">
  <a href="https://github.com/avp-protocol/spec">AVP Specification</a> ·
  <a href="https://github.com/avp-protocol/nexusclaw">NexusClaw</a> ·
  <a href="https://github.com/avp-protocol/avp-hardware">AVP Hardware</a>
</p>

<p align="center">
  <sub>Part of the <a href="https://github.com/avp-protocol">Agent Vault Protocol</a> ecosystem</sub>
</p>
