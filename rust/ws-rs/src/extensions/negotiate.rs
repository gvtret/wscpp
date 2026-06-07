/// Default `Sec-WebSocket-Extensions` offer for permessage-deflate (no context takeover).
pub const OFFER: &str =
    "permessage-deflate; client_no_context_takeover; server_no_context_takeover";

/// Return true when the extensions header includes `permessage-deflate`.
pub fn header_offers_permessage_deflate(extensions_header: &str) -> bool {
    extensions_header.contains("permessage-deflate")
}
