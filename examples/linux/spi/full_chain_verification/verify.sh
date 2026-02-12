#!/usr/bin/env bash

set -e

if [[ $# -ne 1 ]]; then
    echo "TROPIC01 full certificate chain verification example."
    echo "This script verifies certificates downloaded from TROPIC01."
    echo
    echo "Usage:"
    echo "$0 <path_to_certs_from_tropic01>"
    echo
    echo "Hint: Use provided C application to download the certificates from TROPIC01 chip."
    exit 1
fi

# Location of certs downloaded from TROPIC01.
T01_CERTS_DIR=${1%/} # Remove last trailing slash (if any present)

# Check if files exist.
if ! { \
    ls "$T01_CERTS_DIR/t01_ese_cert.der" >/dev/null && \
    ls "$T01_CERTS_DIR/t01_xxxx_ca_cert.der" >/dev/null && \
    ls "$T01_CERTS_DIR/t01_ca_cert.der" >/dev/null; \
}; then
    echo "Missing required cert(s) in '$T01_CERTS_DIR'. Make sure to run the provided C application to download the certificates from the TROPIC01. Check the tutorials."
    exit 1
fi

# Temp dir we will use for storing downloaded certs.
TMPDIR=$(mktemp -d)
trap 'rm -rf "$TMPDIR"' EXIT

# Download certificate authorities from Tropic Square PKI web
curl --fail http://pki.tropicsquare.com/l0/tropic01_xxxx_ca_certificate_sn_30001.pem -o "$TMPDIR/tropic01_xxxx_ca_certificate_sn_30001.pem"
curl --fail http://pki.tropicsquare.com/l0/tropic01_ca_certificate_sn_3001.pem -o "$TMPDIR/tropic01_ca_certificate_sn_3001.pem"

# Parse CRLs from certificates read from device in previous example
L3=$(openssl x509 -in "$T01_CERTS_DIR/t01_ese_cert.der" -inform DER -text | grep URI | cut -d ':' -f 2-)
L2=$(openssl x509 -in "$T01_CERTS_DIR/t01_xxxx_ca_cert.der" -inform DER -text | grep URI | cut -d ':' -f 2-)
L1=$(openssl x509 -in "$T01_CERTS_DIR/t01_ca_cert.der" -inform DER -text | grep URI | cut -d ':' -f 2-)

# Download CRLs
curl --fail "$L3" -o "$TMPDIR/t01-Tv1.crl" # Downloads t01-Tv1.crl
curl --fail "$L2" -o "$TMPDIR/t01v1.crl" # Downloads t01v1.crl
curl --fail "$L1" -o "$TMPDIR/tsrv1.crl" # Downloads tsrv1.crl

# Verify (chip) device certificate
echo "Verifying (chip) device certificate..."
cat "$TMPDIR/tropic01_xxxx_ca_certificate_sn_30001.pem" \
    "$TMPDIR/t01-Tv1.crl" \
    "$TMPDIR/tropic01_ca_certificate_sn_3001.pem" \
    "$TMPDIR/t01v1.crl" \
    "./tropicsquare_root_ca_certificate_sn_301.pem" \
    "$TMPDIR/tsrv1.crl" > "$TMPDIR/chain.pem"
openssl verify -verbose -crl_check -CAfile "$TMPDIR/chain.pem" "$T01_CERTS_DIR/t01_ese_cert.der"

# Verify the "Part Number (group)" certificate
echo "Verifying 'Part Number (group)' certificate..."
cat "$TMPDIR/tropic01_ca_certificate_sn_3001.pem" \
    "$TMPDIR/t01v1.crl" \
    "./tropicsquare_root_ca_certificate_sn_301.pem" \
    "$TMPDIR/tsrv1.crl" > "$TMPDIR/chain.pem"
openssl verify -verbose -crl_check -CAfile "$TMPDIR/chain.pem" "$T01_CERTS_DIR/t01_xxxx_ca_cert.der"

# Verify the "Product (Part Name)" certificate
echo "Verifying 'Product (Part Name)' certificate..."
cat "./tropicsquare_root_ca_certificate_sn_301.pem" "$TMPDIR/tsrv1.crl" > "$TMPDIR/chain.pem"
openssl verify -verbose -crl_check -CAfile "$TMPDIR/chain.pem" "$T01_CERTS_DIR/t01_ca_cert.der"

# Verify Tropic Square Root Certificate
echo "Verifying Tropic Square Root certificate..."

# We provide the root certificate here, in this repository, for convenience.
# HOWEVER. Do not blindly trust this certificate file from GitHub alone.
# To protect against repository compromise, you must verify the certificate
# fingerprint through an independent channel. Contact Tropic Square directly
# to obtain the verified fingerprint.
openssl verify -verbose -CAfile "./tropicsquare_root_ca_certificate_sn_301.pem" "./tropicsquare_root_ca_certificate_sn_301.pem"

echo "All certificates verified successfully!"