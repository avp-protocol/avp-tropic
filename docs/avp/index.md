# AVP-Tropic Firmware Documentation

## Overview

AVP-Tropic is the reference firmware implementation of the **Agent Vault Protocol (AVP)** Hardware conformance level, using the **TROPIC01** secure element from Tropic Square.

This firmware enables AI agents to securely store and manage credentials with hardware-grade protection. Keys are generated, stored, and used entirely within the tamper-resistant TROPIC01 silicon — they never touch host memory.

## Documentation Index

| Document | Description |
|----------|-------------|
| [Architecture](architecture.md) | System architecture and security model |
| [API Reference](api-reference.md) | Complete API documentation |
| [HAL Guide](hal-guide.md) | Hardware abstraction layer guide |
| [Build Guide](build-guide.md) | Building and flashing the firmware |
| [Integration Guide](integration-guide.md) | Integrating with AI agent frameworks |

## Quick Links

- [AVP Specification](https://github.com/avp-protocol/spec)
- [TROPIC01 Documentation](https://tropicsquare.github.io/libtropic/)
- [Source Code](https://github.com/avp-protocol/avp-tropic)

## Features

### AVP Conformance Level: Hardware

| Capability | Status |
|------------|--------|
| Core Operations (STORE, RETRIEVE) | ✅ Implemented |
| Full Operations (all 7 ops) | ✅ Implemented |
| Hardware Extension (HW_*) | ✅ Implemented |
| Attestation | ✅ TROPIC01 certificates |
| FIPS 140-3 Level 3 | ✅ Via TROPIC01 |

### Supported Platforms

| Platform | MCU | Status |
|----------|-----|--------|
| NUCLEO-U575ZI-Q | STM32U575 | ✅ Reference |
| NUCLEO-U585AI-Q | STM32U585 | ✅ Compatible |
| Custom STM32U5 | Any STM32U5 | ✅ With HAL config |

### Security Features

- **Hardware RNG** — STM32U5 TRNG for session IDs and nonces
- **TrustZone** — Optional secure/non-secure isolation
- **Encrypted SPI** — TROPIC01 session encryption
- **PIN Authentication** — Keccak-based PIN verification
- **Tamper Detection** — TROPIC01 anti-tamper response
