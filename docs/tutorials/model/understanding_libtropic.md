# 2. Understanding Libtropic

This chapter is a short TL;DR that helps you orient quickly so you can continue with the next tutorials.
It intentionally links to deeper reference pages instead of duplicating them.

If you want the authoritative chip-level details (command semantics, memory layout, security model), refer to the **TROPIC01 documentation** in the [TROPIC01 repository](https://github.com/tropicsquare/tropic01).

- In particular, look for the **Datasheet** and **User API** documents.

---

## Communication Layers: L2 vs L3
Libtropic communicates with TROPIC01 using a layered protocol described in the reference architecture page.

- Read: [Libtropic Architecture](../../reference/libtropic_architecture.md)

The practical takeaway:

- **L2 (Layer 2) requests do not require a Secure Session.** L2 is unencrypted and is used for chip info and setup tasks (including setting up L3).
- **L3 (Layer 3) commands require an established Secure Session.** L3 traffic is carried over an encrypted Secure Channel session (Noise-based, with forward secrecy).

If you’re unsure which API call is L2 vs L3, the function naming and tutorial text usually hints at it (e.g., “L2 request”, “L3 command”), and the doxygen API reference is the ultimate source.

- Read: [API reference](../../doxygen/build/html/index.html)

---

## Libtropic Architecture in 30 Seconds
At a high level:

- **Public API functions** are in `include/libtropic.h` and implemented in `src/`.
- **HAL (hardware abstration layer of the host platform)** lives in `hal/` and provides the low-level transport (SPI on embedded targets, TCP to the model, etc.).
- **CAL/CFP (crypto abstraction / provider)** lives in `cal/` and plugs in crypto implementations (e.g., MbedTLS/OpenSSL/WolfCrypt/Trezor Crypto).

These are build-time choices when you integrate Libtropic:

- How to build/configure/use: read [Integrating Libtropic](../../reference/integrating_libtropic/index.md)
- Supported host platforms: read [Host Platforms](../../compatibility/host_platforms/index.md)
- Supported crypto providers: read [CFPs](../../compatibility/cfps/index.md)

---

## Secure Session Basics (and Pairing Keys)
Most interesting TROPIC01 features are accessed via **L3**, so you will frequently do this flow:

1. Use L2 requests to read info or reboot to the right mode.
2. Start a Secure Session with a selected **pairing key slot**.
    - We recommend using our helper `lt_verify_chip_and_start_secure_session`, which will handle all L2 communication required to estabilish a Secure Session.
3. Execute L3 commands.
4. Abort the session and deinitialize.

Pairing keys are central to the handshake:

- Read: [Default Pairing Keys](../../reference/default_pairing_keys.md)

Key points:

- New chips come with a factory pairing public key in slot 0 and you use the corresponding private key to establish your *first* Secure Session.
- A common production pattern is: **use slot 0 to bootstrap**, then write your own pairing public key to another slot and **invalidate slot 0**.
    - It is also highly recommended to ensure that your TROPIC01 is genuine by verifying the certificate chain.

!!! danger "Irreversible Actions"
	Some operations are one-way. Invalidating pairing slots, erasing certain chip configuration, and similar actions are **not reversible on a real chip**.
	On the model, you can recover by restarting the model server (fresh state), but do not rely on that behavior for real hardware.

If Secure Session establishment fails, the two most common causes are wrong pairing keys or the chip being in Maintenance Mode:

- Read: [FAQ](../../faq.md#i-cannot-establish-a-secure-session)

---

## Debugging and Observability
When something doesn’t work, you typically want: (1) clear return codes, (2) logs, (3) a way to inspect the transport.

### 1) Return Codes
Most API functions return a `lt_ret_t` error code. The FAQ explains common “surprising” root causes (wiring, maintenance mode, etc.):

- Read: [FAQ](../../faq.md)

### 2) Logging
Libtropic logging is off by default (except tests defaulting to Info). Turn it on during evaluation:

- [Logging guide](../../reference/logging.md)
- [Configuration](../../reference/integrating_libtropic/how_to_configure/index.md) (CMake options)

Useful options:

- `LT_LOG_LVL` to increase verbosity.
- `LT_PRINT_SPI_DATA` if you need to see low-level traffic.

### 3) Debuggers and Sanitizers
If you suspect memory issues or need to step through code:

- [Debugging guide](../../reference/debugging.md)

On Linux, tests against the model can be run with AddressSanitizer or Valgrind (and you can attach `gdb`).

!!! tip "Model + gdb"
	If you want to debug a client binary against the model, run the **model server manually** (as in the first tutorial) and then run your binary under `gdb`.

---

## Safety: Operations that Can Change Chip State Permanently
TROPIC01 exposes powerful commands; some of them intentionally make **permanent** changes.

Examples you will encounter in these tutorials:

- Pairing key writes/invalidations (used in the hardware-wallet tutorial).
- R-config/R-memory erases.
- Firmware update (interruption can brick the device; avoid power loss).

!!! danger "When in doubt, use the model!"
	If you are exploring commands you don’t fully understand yet, prefer using the **TROPIC01 Model** until you’re confident about the effects.

---

## Where to Go Next
- Continue with [Hardware Wallet](hw_wallet.md) to see secure-session + configuration/pairing key management in practice.
- Keep [Libtropic Architecture](../../reference/libtropic_architecture.md) and [Debugging](../../reference/debugging.md) open while working through the rest of the tutorials on the TROPIC01 model.