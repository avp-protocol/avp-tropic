# AVP-Tropic Architecture

## System Overview

```
┌─────────────────────────────────────────────────────────────────────────┐
│                        AI Agent Application                              │
│   ┌─────────────────────────────────────────────────────────────────┐   │
│   │  Agent Framework (ZeroClaw, LangChain, CrewAI, etc.)            │   │
│   │  ┌─────────────────────────────────────────────────────────────┐│   │
│   │  │  avp_retrieve("anthropic_api_key") → API call to Anthropic  ││   │
│   │  └─────────────────────────────────────────────────────────────┘│   │
│   └───────────────────────────────┬─────────────────────────────────┘   │
│                                   │                                      │
│   ┌───────────────────────────────▼─────────────────────────────────┐   │
│   │                     AVP Protocol Layer                           │   │
│   │  ┌─────────┐ ┌──────────────┐ ┌─────────┐ ┌──────────────────┐  │   │
│   │  │DISCOVER │ │AUTHENTICATE  │ │ STORE   │ │    RETRIEVE      │  │   │
│   │  └─────────┘ └──────────────┘ └─────────┘ └──────────────────┘  │   │
│   │  ┌─────────┐ ┌──────────────┐ ┌─────────┐                       │   │
│   │  │ DELETE  │ │    LIST      │ │ ROTATE  │  (7 Core Operations)  │   │
│   │  └─────────┘ └──────────────┘ └─────────┘                       │   │
│   │  ┌─────────────┐ ┌───────────┐ ┌────────────┐                   │   │
│   │  │HW_CHALLENGE │ │  HW_SIGN  │ │ HW_ATTEST  │ (3 HW Extensions) │   │
│   │  └─────────────┘ └───────────┘ └────────────┘                   │   │
│   └───────────────────────────────┬─────────────────────────────────┘   │
│                                   │                                      │
│   ┌───────────────────────────────▼─────────────────────────────────┐   │
│   │                      libtropic SDK                               │   │
│   │  Session Management │ L2/L3 Protocol │ Crypto Abstraction       │   │
│   └───────────────────────────────┬─────────────────────────────────┘   │
│                                   │                                      │
│   ┌───────────────────────────────▼─────────────────────────────────┐   │
│   │              Hardware Abstraction Layer (HAL)                    │   │
│   │  ┌─────────────────────────────────────────────────────────────┐│   │
│   │  │  STM32U5 HAL: SPI, GPIO, RNG, UART (debug)                  ││   │
│   │  └─────────────────────────────────────────────────────────────┘│   │
│   └───────────────────────────────┬─────────────────────────────────┘   │
│                     Host MCU      │                                      │
└───────────────────────────────────┼─────────────────────────────────────┘
                                    │ SPI Bus (encrypted session)
                                    │
┌───────────────────────────────────▼─────────────────────────────────────┐
│                    TROPIC01 Secure Element                               │
│  ┌────────────────────────────────────────────────────────────────────┐ │
│  │                         RISC-V Core                                 │ │
│  │  ┌──────────────┐  ┌──────────────┐  ┌──────────────────────────┐  │ │
│  │  │ Session Mgmt │  │ PIN Verify   │  │ Access Control           │  │ │
│  │  │ (Keccak)     │  │ (Keccak)     │  │ (per-slot permissions)   │  │ │
│  │  └──────────────┘  └──────────────┘  └──────────────────────────┘  │ │
│  └────────────────────────────────────────────────────────────────────┘ │
│  ┌────────────────────────────────────────────────────────────────────┐ │
│  │                    SPECT Crypto Coprocessor                         │ │
│  │  ┌──────────────┐  ┌──────────────┐  ┌──────────────────────────┐  │ │
│  │  │ ECC-P256     │  │ Ed25519      │  │ X25519                   │  │ │
│  │  │ ECDSA Sign   │  │ EdDSA Sign   │  │ Key Exchange             │  │ │
│  │  └──────────────┘  └──────────────┘  └──────────────────────────┘  │ │
│  │  ┌──────────────┐  ┌──────────────┐  ┌──────────────────────────┐  │ │
│  │  │ AES-256-GCM  │  │ SHA-3/Keccak │  │ TRNG + PUF               │  │ │
│  │  │ Encrypt/Dec  │  │ Hash/MAC     │  │ True Random              │  │ │
│  │  └──────────────┘  └──────────────┘  └──────────────────────────┘  │ │
│  └────────────────────────────────────────────────────────────────────┘ │
│  ┌────────────────────────────────────────────────────────────────────┐ │
│  │                    Secure Key Storage                               │ │
│  │  ┌────┐┌────┐┌────┐┌────┐┌────┐┌────┐┌────┐┌────┐    128 slots    │ │
│  │  │ 0  ││ 1  ││ 2  ││ 3  ││ 4  ││ 5  ││... ││127 │    per slot:    │ │
│  │  │key ││key ││key ││key ││data││data││    ││    │    - 256B data  │ │
│  │  └────┘└────┘└────┘└────┘└────┘└────┘└────┘└────┘    - metadata   │ │
│  └────────────────────────────────────────────────────────────────────┘ │
│                    Tamper-Resistant Silicon Boundary                    │
└─────────────────────────────────────────────────────────────────────────┘
```

## Layer Descriptions

### 1. AVP Protocol Layer (`avp/`)

The AVP Protocol Layer implements the Agent Vault Protocol specification. It provides:

- **Session Management** — AVP session lifecycle (AUTHENTICATE → operations → timeout)
- **Secret Name Mapping** — Maps AVP secret names to TROPIC01 slot indices
- **Operation Routing** — Routes AVP operations to appropriate libtropic calls
- **Error Translation** — Converts libtropic errors to AVP error codes

**Key Files:**
- `avp/avp_tropic.h` — Public API header
- `avp/avp_tropic.c` — Implementation

### 2. libtropic SDK (`src/`, `include/`)

The libtropic SDK (from Tropic Square) handles low-level communication with TROPIC01:

- **L1 Layer** — SPI framing and transport
- **L2 Layer** — Session encryption and authentication
- **L3 Layer** — Command/response protocol
- **Crypto Abstraction** — Pluggable crypto backends

### 3. Hardware Abstraction Layer (`hal/`)

The HAL provides platform-specific implementations:

- **SPI Communication** — Full-duplex SPI to TROPIC01
- **GPIO Control** — Chip select, interrupt, reset pins
- **Random Number Generation** — Hardware RNG for session nonces
- **Timing** — Delays and timeouts

**Supported HALs:**
- `hal/stm32/stm32u5_tropic_click/` — STM32U5 + MikroE Click

## Data Flow

### STORE Operation

```
Agent                    AVP Layer              libtropic             TROPIC01
  │                          │                      │                     │
  │  avp_store("api_key",    │                      │                     │
  │            value)        │                      │                     │
  │─────────────────────────>│                      │                     │
  │                          │                      │                     │
  │                          │  Validate name       │                     │
  │                          │  Map name → slot     │                     │
  │                          │                      │                     │
  │                          │  lt_r_mem_data_write │                     │
  │                          │─────────────────────>│                     │
  │                          │                      │                     │
  │                          │                      │  L2: Encrypt data   │
  │                          │                      │  L1: SPI transfer   │
  │                          │                      │────────────────────>│
  │                          │                      │                     │
  │                          │                      │                     │ Store in
  │                          │                      │                     │ secure slot
  │                          │                      │                     │
  │                          │                      │<────────────────────│
  │                          │                      │  L1: SPI response   │
  │                          │                      │  L2: Verify MAC     │
  │                          │<─────────────────────│                     │
  │                          │  LT_OK               │                     │
  │<─────────────────────────│                      │                     │
  │  AVP_OK                  │                      │                     │
```

### HW_SIGN Operation (Key Never Exported)

```
Agent                    AVP Layer              libtropic             TROPIC01
  │                          │                      │                     │
  │  avp_hw_sign("key",      │                      │                     │
  │              data)       │                      │                     │
  │─────────────────────────>│                      │                     │
  │                          │                      │                     │
  │                          │  lt_ecc_ecdsa_sign   │                     │
  │                          │─────────────────────>│                     │
  │                          │                      │                     │
  │                          │                      │  Send data hash     │
  │                          │                      │────────────────────>│
  │                          │                      │                     │
  │                          │                      │                     │ Sign with
  │                          │                      │                     │ internal key
  │                          │                      │                     │ (never exported)
  │                          │                      │                     │
  │                          │                      │<────────────────────│
  │                          │                      │  Return signature   │
  │                          │<─────────────────────│                     │
  │<─────────────────────────│                      │                     │
  │  signature (64 bytes)    │                      │                     │
  │                          │                      │                     │
  │  Key NEVER left TROPIC01 │                      │                     │
```

## Security Boundaries

### Trust Zones

```
┌─────────────────────────────────────────────────────────────────┐
│                      UNTRUSTED ZONE                              │
│  ┌───────────────────────────────────────────────────────────┐  │
│  │  Network, Filesystem, User Input, Other Processes         │  │
│  └───────────────────────────────────────────────────────────┘  │
└───────────────────────────────────────────────────────────────┬─┘
                                                                │
┌───────────────────────────────────────────────────────────────▼─┐
│                      HOST MCU (Partially Trusted)                │
│  ┌───────────────────────────────────────────────────────────┐  │
│  │  AVP Layer: Handles encrypted data, never sees plaintext  │  │
│  │  keys. Session tokens are ephemeral.                      │  │
│  └───────────────────────────────────────────────────────────┘  │
│  ┌───────────────────────────────────────────────────────────┐  │
│  │  STM32U5 TrustZone (optional): Isolate AVP code in        │  │
│  │  secure world, prevent other code from accessing SPI.     │  │
│  └───────────────────────────────────────────────────────────┘  │
└───────────────────────────────────────────────────────────────┬─┘
                                                                │
┌───────────────────────────────────────────────────────────────▼─┐
│                      TROPIC01 (Fully Trusted)                    │
│  ┌───────────────────────────────────────────────────────────┐  │
│  │  - Keys generated internally (PUF + TRNG)                 │  │
│  │  - Keys stored in tamper-resistant memory                 │  │
│  │  - Crypto operations performed internally                 │  │
│  │  - Keys NEVER exported in plaintext                       │  │
│  │  - Tamper detection → automatic key zeroization           │  │
│  └───────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────┘
```

### Attack Surface Analysis

| Attack Vector | Mitigation |
|---------------|------------|
| SPI bus sniffing | L2 session encryption (AES-256-GCM) |
| Replay attacks | Session nonces, sequence numbers |
| PIN brute force | Exponential backoff, lockout after N attempts |
| Firmware tampering | Secure boot, signed firmware |
| Physical probing | Tamper mesh, active shield |
| Side-channel (power) | Constant-time operations in SPECT |
| Cold boot | Keys in non-volatile secure memory |

## Memory Layout

### Host MCU (STM32U5)

```
0x0800_0000 ┌─────────────────────────┐
            │   Vector Table          │
0x0800_0200 ├─────────────────────────┤
            │   Firmware Code         │
            │   - AVP Protocol        │
            │   - libtropic           │
            │   - HAL                 │
0x0802_0000 ├─────────────────────────┤
            │   Read-Only Data        │
0x0803_0000 ├─────────────────────────┤
            │   Reserved              │
0x0804_0000 └─────────────────────────┘

0x2000_0000 ┌─────────────────────────┐
            │   Stack                 │
0x2000_4000 ├─────────────────────────┤
            │   Heap                  │
0x2000_8000 ├─────────────────────────┤
            │   BSS (Zero-init)       │
0x2000_C000 ├─────────────────────────┤
            │   Data                  │
0x2001_0000 └─────────────────────────┘
```

### TROPIC01 Slot Layout

```
Slot 0-31:   ECC Key Pairs (P-256, Ed25519)
             - Private key (32 bytes, never exported)
             - Public key (64 bytes, exportable)
             - Key attributes

Slot 32-95:  Symmetric Keys
             - AES-256 keys (32 bytes)
             - Key attributes

Slot 96-127: Data Slots (for AVP secrets)
             - Up to 256 bytes per slot
             - Encrypted with slot-specific key
             - Metadata (name hash, timestamps)
```

## Configuration Options

### Compile-Time Options

| Option | Default | Description |
|--------|---------|-------------|
| `LT_USE_INT_PIN` | 1 | Use interrupt pin for faster response |
| `LT_LOG_LEVEL` | 2 | Log level (0=none, 4=debug) |
| `AVP_MAX_SECRETS` | 32 | Maximum number of AVP secrets |
| `AVP_SESSION_TTL` | 300 | Default session timeout (seconds) |

### Runtime Configuration

```c
/* Device configuration */
lt_dev_stm32u5_tropic_click_t device = {
    .spi_instance = SPI1,
    .baudrate_prescaler = SPI_BAUDRATEPRESCALER_16,  /* 10 MHz */
    .spi_cs_gpio_pin = GPIO_PIN_14,
    .spi_cs_gpio_port = GPIOD,
    .int_gpio_pin = GPIO_PIN_13,
    .int_gpio_port = GPIOF,
    .rng_handle = &hrng,
};
```
