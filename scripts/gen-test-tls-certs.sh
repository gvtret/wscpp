#!/usr/bin/env bash
# Generate local TLS fixtures for wscpp tests (never commit the output).
#
# Usage: scripts/gen-test-tls-certs.sh [output_dir]
# Default output: tests/tls/fixtures/
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
out_dir="${1:-${root}/tests/tls/fixtures}"
days=825
ca_subj="/CN=WSCPP Test CA/O=wscpp-tests/C=XX"
server_subj="/CN=localhost/O=wscpp-tests/C=XX"
client_subj="/CN=wscpp-test-client/O=wscpp-tests/C=XX"

if ! command -v openssl >/dev/null 2>&1; then
    echo "openssl not found" >&2
    exit 1
fi

mkdir -p "${out_dir}"
work="$(mktemp -d)"
trap 'rm -rf "${work}"' EXIT

cat > "${work}/server-ext.cnf" <<'EOF'
subjectAltName = DNS:localhost, IP:127.0.0.1
extendedKeyUsage = serverAuth
EOF

cat > "${work}/client-ext.cnf" <<'EOF'
extendedKeyUsage = clientAuth
EOF

echo "Generating test CA in ${out_dir} (local only — do not commit)"

openssl genrsa -out "${out_dir}/ca.key" 4096
chmod 600 "${out_dir}/ca.key"
openssl req -new -x509 -days "${days}" -key "${out_dir}/ca.key" \
    -out "${out_dir}/ca.crt" -subj "${ca_subj}"

openssl genrsa -out "${out_dir}/server.key" 2048
chmod 600 "${out_dir}/server.key"
openssl req -new -key "${out_dir}/server.key" -out "${work}/server.csr" -subj "${server_subj}"
openssl x509 -req -days "${days}" \
    -in "${work}/server.csr" \
    -CA "${out_dir}/ca.crt" -CAkey "${out_dir}/ca.key" -CAcreateserial \
    -out "${out_dir}/server.crt" \
    -extfile "${work}/server-ext.cnf"

openssl genrsa -out "${out_dir}/client.key" 2048
chmod 600 "${out_dir}/client.key"
openssl req -new -key "${out_dir}/client.key" -out "${work}/client.csr" -subj "${client_subj}"
openssl x509 -req -days "${days}" \
    -in "${work}/client.csr" \
    -CA "${out_dir}/ca.crt" -CAkey "${out_dir}/ca.key" -CAcreateserial \
    -out "${out_dir}/client.crt" \
    -extfile "${work}/client-ext.cnf"

rm -f "${out_dir}/"*.csr "${out_dir}/"*.srl 2>/dev/null || true

echo "Created:"
echo "  ${out_dir}/ca.crt      ${out_dir}/ca.key"
echo "  ${out_dir}/server.crt  ${out_dir}/server.key"
echo "  ${out_dir}/client.crt  ${out_dir}/client.key"
echo
echo "Run TLS tests: cd build && ctest -R Tls --output-on-failure"
