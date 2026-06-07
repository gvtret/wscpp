# TLS / WSS for wscpp

wscpp supports `wss://` (RFC 2818) on both transports:

- **ASIO** — `asio::ssl::context` via `server::set_ssl_context()` / `client::set_ssl_context()`
- **Linux POSIX** — `wscpp::net::openssl_context` with the same API surface

Public I/O uses `std::error_code`; TLS setup failures return `false` or error codes from `connect()`.

## Quick start (development)

For local examples (`examples/tls_server`, `examples/tls_client`) you can use a single self-signed server certificate:

```bash
bash scripts/gen-test-tls-certs.sh
# copies layout into tests/tls/fixtures/ — use server.crt + server.key for examples:
examples/tls_server/server --cert tests/tls/fixtures/server.crt --key tests/tls/fixtures/server.key
examples/tls_client/client wss://127.0.0.1:8443/
```

The client example uses `verify_none` by default (suitable for dev self-signed certs).

## Test PKI layout (local only)

Integration tests expect a **small test CA** and leaf certificates under `tests/tls/fixtures/`:

| File | Purpose |
|------|---------|
| `ca.crt` / `ca.key` | Test certificate authority |
| `server.crt` / `server.key` | Server leaf (SAN: `localhost`, `127.0.0.1`) |
| `client.crt` / `client.key` | Client leaf (for mutual TLS tests) |

**These files are gitignored and must not be committed.**

Generate everything with one command:

```bash
bash scripts/gen-test-tls-certs.sh
# optional custom directory:
bash scripts/gen-test-tls-certs.sh /tmp/wscpp-tls-fixtures
export WSCPP_TLS_FIXTURES_DIR=/tmp/wscpp-tls-fixtures
```

Run TLS tests:

```bash
cmake -B build -DWSCPP_BUILD_TESTS=ON
cmake --build build
bash scripts/gen-test-tls-certs.sh
cd build && ctest -R TlsIntegration --output-on-failure
```

If fixtures are missing, TLS tests **skip** with a message pointing to the script above.

## Manual PKI creation (OpenSSL)

Equivalent steps to the generator script:

### 1. Test CA

```bash
mkdir -p tests/tls/fixtures
cd tests/tls/fixtures

openssl genrsa -out ca.key 4096
chmod 600 ca.key
openssl req -new -x509 -days 825 -key ca.key -out ca.crt \
  -subj "/CN=WSCPP Test CA/O=wscpp-tests/C=XX"
```

### 2. Server certificate

```bash
openssl genrsa -out server.key 2048
chmod 600 server.key
openssl req -new -key server.key -out server.csr \
  -subj "/CN=localhost/O=wscpp-tests/C=XX"

cat > server-ext.cnf <<'EOF'
subjectAltName = DNS:localhost, IP:127.0.0.1
extendedKeyUsage = serverAuth
EOF

openssl x509 -req -days 825 -in server.csr \
  -CA ca.crt -CAkey ca.key -CAcreateserial \
  -out server.crt -extfile server-ext.cnf
```

### 3. Client certificate

```bash
openssl genrsa -out client.key 2048
chmod 600 client.key
openssl req -new -key client.key -out client.csr \
  -subj "/CN=wscpp-test-client/O=wscpp-tests/C=XX"

cat > client-ext.cnf <<'EOF'
extendedKeyUsage = clientAuth
EOF

openssl x509 -req -days 825 -in client.csr \
  -CA ca.crt -CAkey ca.key -CAcreateserial \
  -out client.crt -extfile client-ext.cnf

rm -f *.csr *.srl
```

## What the TLS tests cover

| Test | Server | Client |
|------|--------|--------|
| `WssEchoWithServerCertificate` | server cert, verify optional | default (`verify_none`) |
| `WssEchoWithCaVerification` | server cert | trusts `ca.crt`, verifies server |
| `WssMutualTlsEcho` | requires client cert | presents client cert + verifies server |

## Security notes

- Test CA and keys are for **localhost testing only** — not for production.
- Never commit `*.key`, `*.pem`, `*.crt`, or contents of `tests/tls/fixtures/`.
- CI may omit TLS fixtures; skipped tests are expected unless the job runs `gen-test-tls-certs.sh`.
