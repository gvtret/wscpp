//! WebSocket extensions (RFC 7692 permessage-deflate when feature `deflate` is on).

/// Extension negotiation helpers (`Sec-WebSocket-Extensions` header).
pub mod negotiate;

#[cfg(feature = "deflate")]
/// RFC 7692 permessage-deflate compress/decompress.
pub mod permessage_deflate;
