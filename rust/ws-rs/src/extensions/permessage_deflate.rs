use crate::error::{Error, Result};

pub use crate::extensions::negotiate::{header_offers_permessage_deflate, OFFER};

fn ends_with_deflate_tail(data: &[u8]) -> bool {
    data.len() >= 4
        && data[data.len() - 4] == 0x00
        && data[data.len() - 3] == 0x00
        && data[data.len() - 2] == 0xff
        && data[data.len() - 1] == 0xff
}

/// Compress one message (raw deflate, RFC 7692).
pub fn compress_message(data: &[u8]) -> Result<Vec<u8>> {
    if data.is_empty() {
        return Ok(Vec::new());
    }

    let mut compress = flate2::Compress::new(flate2::Compression::default(), false);
    let mut out = Vec::with_capacity(data.len() + 32);
    compress
        .compress_vec(data, &mut out, flate2::FlushCompress::Sync)
        .map_err(|_| Error::Protocol("permessage-deflate compression failed".into()))?;

    if out.len() < 4 || !ends_with_deflate_tail(&out) {
        return Err(Error::Protocol(
            "permessage-deflate compression failed".into(),
        ));
    }

    out.truncate(out.len() - 4);
    Ok(out)
}

/// Decompress one message (raw deflate, RFC 7692).
pub fn decompress_message(data: &[u8]) -> Result<Vec<u8>> {
    if data.is_empty() {
        return Ok(Vec::new());
    }

    let mut input = Vec::with_capacity(data.len() + 4);
    input.extend_from_slice(data);
    input.extend_from_slice(&[0x00, 0x00, 0xff, 0xff]);

    let mut decompress = flate2::Decompress::new(false);
    let mut out = Vec::with_capacity(data.len() * 8 + 64);
    decompress
        .decompress_vec(&input, &mut out, flate2::FlushDecompress::Sync)
        .map_err(|_| Error::Protocol("permessage-deflate decompression failed".into()))?;

    Ok(out)
}
