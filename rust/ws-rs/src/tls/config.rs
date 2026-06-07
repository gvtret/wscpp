use std::fs::File;
use std::io::BufReader;
use std::path::Path;
use std::sync::Arc;

use rustls::client::danger::{HandshakeSignatureValid, ServerCertVerified, ServerCertVerifier};
use rustls::pki_types::{CertificateDer, PrivateKeyDer, ServerName, UnixTime};
use rustls::{ClientConfig, DigitallySignedStruct, RootCertStore, ServerConfig, SignatureScheme};

use crate::error::{Error, Result};

/// Client TLS settings (RFC 2818 / wss://).
#[derive(Clone)]
pub struct ClientTlsConfig {
    pub(crate) config: Arc<ClientConfig>,
}

/// Server TLS settings for WSS.
#[derive(Clone)]
pub struct ServerTlsConfig {
    pub(crate) config: Arc<ServerConfig>,
}

impl ClientTlsConfig {
    /// Accept any server certificate (default for `wss://`, mirrors C++ `verify_none`).
    pub fn insecure() -> Self {
        let config = ClientConfig::builder()
            .dangerous()
            .with_custom_certificate_verifier(Arc::new(SkipServerVerification))
            .with_no_client_auth();
        Self {
            config: Arc::new(config),
        }
    }

    /// Trust a CA and verify server certificates (SNI hostname must match).
    pub fn from_ca_pem(ca_path: impl AsRef<Path>) -> Result<Self> {
        let mut roots = RootCertStore::empty();
        for cert in load_certs(ca_path)? {
            roots.add(cert).map_err(tls_err)?;
        }
        let config = ClientConfig::builder()
            .with_root_certificates(roots)
            .with_no_client_auth();
        Ok(Self {
            config: Arc::new(config),
        })
    }

    /// Mutual TLS: client cert + CA for server verification.
    pub fn mutual(
        ca_path: impl AsRef<Path>,
        cert_path: impl AsRef<Path>,
        key_path: impl AsRef<Path>,
    ) -> Result<Self> {
        let mut roots = RootCertStore::empty();
        for cert in load_certs(ca_path)? {
            roots.add(cert).map_err(tls_err)?;
        }
        let certs = load_certs(cert_path)?;
        let key = load_key(key_path)?;
        let config = ClientConfig::builder()
            .with_root_certificates(roots)
            .with_client_auth_cert(certs, key)
            .map_err(tls_err)?;
        Ok(Self {
            config: Arc::new(config),
        })
    }
}

impl ServerTlsConfig {
    /// Load server certificate chain and private key (PEM).
    pub fn from_pem(cert_path: impl AsRef<Path>, key_path: impl AsRef<Path>) -> Result<Self> {
        let certs = load_certs(cert_path)?;
        let key = load_key(key_path)?;
        let config = ServerConfig::builder()
            .with_no_client_auth()
            .with_single_cert(certs, key)
            .map_err(tls_err)?;
        Ok(Self {
            config: Arc::new(config),
        })
    }

    /// Require client certificates signed by the given CA.
    pub fn from_pem_with_client_ca(
        cert_path: impl AsRef<Path>,
        key_path: impl AsRef<Path>,
        ca_path: impl AsRef<Path>,
    ) -> Result<Self> {
        let certs = load_certs(cert_path)?;
        let key = load_key(key_path)?;
        let mut client_roots = RootCertStore::empty();
        for cert in load_certs(ca_path)? {
            client_roots.add(cert).map_err(tls_err)?;
        }
        let verifier = rustls::server::WebPkiClientVerifier::builder(Arc::new(client_roots))
            .build()
            .map_err(tls_err)?;
        let config = ServerConfig::builder()
            .with_client_cert_verifier(verifier)
            .with_single_cert(certs, key)
            .map_err(tls_err)?;
        Ok(Self {
            config: Arc::new(config),
        })
    }
}

pub(crate) fn load_certs(path: impl AsRef<Path>) -> Result<Vec<CertificateDer<'static>>> {
    let file = File::open(path.as_ref()).map_err(|e| Error::Tls(e.to_string()))?;
    let mut reader = BufReader::new(file);
    let certs: Vec<_> = rustls_pemfile::certs(&mut reader)
        .collect::<std::result::Result<Vec<_>, _>>()
        .map_err(|e| Error::Tls(e.to_string()))?;
    if certs.is_empty() {
        return Err(Error::Tls("no certificates in PEM file".into()));
    }
    Ok(certs)
}

pub(crate) fn load_key(path: impl AsRef<Path>) -> Result<PrivateKeyDer<'static>> {
    let file = File::open(path.as_ref()).map_err(|e| Error::Tls(e.to_string()))?;
    let mut reader = BufReader::new(file);
    if let Some(key) = rustls_pemfile::pkcs8_private_keys(&mut reader).next() {
        let key = key.map_err(|e| Error::Tls(e.to_string()))?;
        return Ok(PrivateKeyDer::Pkcs8(key));
    }
    let file = File::open(path.as_ref()).map_err(|e| Error::Tls(e.to_string()))?;
    let mut reader = BufReader::new(file);
    if let Some(key) = rustls_pemfile::rsa_private_keys(&mut reader).next() {
        let key = key.map_err(|e| Error::Tls(e.to_string()))?;
        return Ok(PrivateKeyDer::Pkcs1(key));
    }
    Err(Error::Tls("no private key in PEM file".into()))
}

fn tls_err(e: impl std::fmt::Display) -> Error {
    Error::Tls(e.to_string())
}

#[derive(Debug)]
struct SkipServerVerification;

impl ServerCertVerifier for SkipServerVerification {
    fn verify_server_cert(
        &self,
        _end_entity: &CertificateDer<'_>,
        _intermediates: &[CertificateDer<'_>],
        _server_name: &ServerName<'_>,
        _ocsp_response: &[u8],
        _now: UnixTime,
    ) -> std::result::Result<ServerCertVerified, rustls::Error> {
        Ok(ServerCertVerified::assertion())
    }

    fn verify_tls12_signature(
        &self,
        _message: &[u8],
        _cert: &CertificateDer<'_>,
        _dss: &DigitallySignedStruct,
    ) -> std::result::Result<HandshakeSignatureValid, rustls::Error> {
        Ok(HandshakeSignatureValid::assertion())
    }

    fn verify_tls13_signature(
        &self,
        _message: &[u8],
        _cert: &CertificateDer<'_>,
        _dss: &DigitallySignedStruct,
    ) -> std::result::Result<HandshakeSignatureValid, rustls::Error> {
        Ok(HandshakeSignatureValid::assertion())
    }

    fn supported_verify_schemes(&self) -> Vec<SignatureScheme> {
        vec![
            SignatureScheme::RSA_PKCS1_SHA256,
            SignatureScheme::ECDSA_NISTP256_SHA256,
            SignatureScheme::RSA_PKCS1_SHA384,
            SignatureScheme::ECDSA_NISTP384_SHA384,
            SignatureScheme::RSA_PKCS1_SHA512,
            SignatureScheme::ECDSA_NISTP521_SHA512,
            SignatureScheme::RSA_PSS_SHA256,
            SignatureScheme::RSA_PSS_SHA384,
            SignatureScheme::RSA_PSS_SHA512,
            SignatureScheme::ED25519,
        ]
    }
}
