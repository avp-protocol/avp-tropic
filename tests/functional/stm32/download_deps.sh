#!/usr/bin/env bash

set -e

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

rm -rf "$SCRIPT_DIR/_deps"
mkdir -p "$SCRIPT_DIR/_deps"

echo "Downloading ed25519..."
curl -L -o "$SCRIPT_DIR/_deps/ed25519.zip" "https://github.com/orlp/ed25519/archive/b1f19fab4aebe607805620d25a5e42566ce46a0e.zip"

# Verify ed25519 zip checksum
echo "Verifying ed25519.zip checksum..."
EXPECTED_ED25519="75f39e64f22ec7474e7881315a3f9135afe6c990737388fb16a6950911b55721"
ACTUAL_ED25519=$(sha256sum "$SCRIPT_DIR/_deps/ed25519.zip" | awk '{print $1}')
if [ "$EXPECTED_ED25519" != "$ACTUAL_ED25519" ]; then
  echo "Checksum mismatch for ed25519.zip: expected $EXPECTED_ED25519, got $ACTUAL_ED25519" >&2
  exit 1
fi

unzip "$SCRIPT_DIR/_deps/ed25519.zip" -d "$SCRIPT_DIR/_deps"
mv "$SCRIPT_DIR/_deps/ed25519-b1f19fab4aebe607805620d25a5e42566ce46a0e" "$SCRIPT_DIR/_deps/ed25519"
rm "$SCRIPT_DIR/_deps/ed25519.zip"

echo "Downloading Micro ECC..."
curl -L -o "$SCRIPT_DIR/_deps/micro-ecc.zip" "https://github.com/kmackay/micro-ecc/archive/refs/tags/v1.1.zip"

# Verify micro-ecc zip checksum
echo "Verifying micro-ecc.zip checksum..."
EXPECTED_MICRO_ECC="67cc3867dda3860335780ddd8004d69d82afeaac8c0aa630e29112a2b5be153d"
ACTUAL_MICRO_ECC=$(sha256sum "$SCRIPT_DIR/_deps/micro-ecc.zip" | awk '{print $1}')
if [ "$EXPECTED_MICRO_ECC" != "$ACTUAL_MICRO_ECC" ]; then
  echo "Checksum mismatch for micro-ecc.zip: expected $EXPECTED_MICRO_ECC, got $ACTUAL_MICRO_ECC" >&2
  exit 1
fi

unzip "$SCRIPT_DIR/_deps/micro-ecc.zip" -d "$SCRIPT_DIR/_deps"
mv "$SCRIPT_DIR/_deps/micro-ecc-1.1" "$SCRIPT_DIR/_deps/micro-ecc"
rm "$SCRIPT_DIR/_deps/micro-ecc.zip"

echo "Downloading MbedTLSv4..."
curl -L -o "$SCRIPT_DIR/_deps/mbedtls.tar.bz2" "https://github.com/Mbed-TLS/mbedtls/releases/download/mbedtls-4.0.0/mbedtls-4.0.0.tar.bz2"

# Verify mbedtls tar.bz2 checksum
echo "Verifying mbedtls.tar.bz2 checksum..."
EXPECTED_MBEDTLS="2f3a47f7b3a541ddef450e4867eeecb7ce2ef7776093f3a11d6d43ead6bf2827"
ACTUAL_MBEDTLS=$(sha256sum "$SCRIPT_DIR/_deps/mbedtls.tar.bz2" | awk '{print $1}')
if [ "$EXPECTED_MBEDTLS" != "$ACTUAL_MBEDTLS" ]; then
  echo "Checksum mismatch for mbedtls.tar.bz2: expected $EXPECTED_MBEDTLS, got $ACTUAL_MBEDTLS" >&2
  exit 1
fi

tar -xjf "$SCRIPT_DIR/_deps/mbedtls.tar.bz2" -C "$SCRIPT_DIR/_deps"
rm "$SCRIPT_DIR/_deps/mbedtls.tar.bz2"
mv "$SCRIPT_DIR/_deps/mbedtls-4.0.0" "$SCRIPT_DIR/_deps/mbedtls_v4"

echo "Downloading WolfSSL..."
curl -L -o "$SCRIPT_DIR/_deps/wolfssl.zip" "https://github.com/wolfSSL/wolfssl/archive/refs/tags/v5.8.4-stable.zip"

echo "Verifying wolfssl.zip checksum..."
EXPECTED_WOLFSSL="9f52b92b2937acdbb03f2a731160d70f23f74a375f651de057214783c266fbeb"
ACTUAL_WOLFSSL=$(sha256sum "$SCRIPT_DIR/_deps/wolfssl.zip" | awk '{print $1}')
if [ "$EXPECTED_WOLFSSL" != "$ACTUAL_WOLFSSL" ]; then
  echo "Checksum mismatch for wolfssl.zip: expected $EXPECTED_WOLFSSL, got $ACTUAL_WOLFSSL" >&2
  exit 1
fi

unzip "$SCRIPT_DIR/_deps/wolfssl.zip" -d "$SCRIPT_DIR/_deps"
mv "$SCRIPT_DIR/_deps/wolfssl-5.8.4-stable" "$SCRIPT_DIR/_deps/wolfssl"
rm "$SCRIPT_DIR/_deps/wolfssl.zip"