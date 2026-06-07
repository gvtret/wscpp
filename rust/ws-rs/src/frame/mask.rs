//! RFC 6455 payload mask/unmask — ported from `wscpp/detail/mask.hpp`.

#[inline]
fn load_u32_le(p: &[u8]) -> u32 {
    u32::from_le_bytes([p[0], p[1], p[2], p[3]])
}

#[inline]
fn store_u32_le(p: &mut [u8], v: u32) {
    p.copy_from_slice(&v.to_le_bytes());
}

#[inline]
fn load_u64_le(p: &[u8]) -> u64 {
    u64::from_le_bytes([p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7]])
}

#[inline]
fn store_u64_le(p: &mut [u8], v: u64) {
    p.copy_from_slice(&v.to_le_bytes());
}

#[inline]
fn mask_key64(key32: u32) -> u64 {
    key32 as u64 | ((key32 as u64) << 32)
}

/// RFC 6455 payload mask/unmask in place (offset 0).
pub fn apply_mask(data: &mut [u8], key: &[u8; 4]) {
    if data.is_empty() {
        return;
    }

    let mut i = 0usize;
    while i < data.len() && (i & 3) != 0 {
        data[i] ^= key[i & 3];
        i += 1;
    }

    let key32 = load_u32_le(key);
    let key64 = mask_key64(key32);

    while i + 64 <= data.len() {
        for chunk in data[i..i + 64].chunks_mut(8) {
            let v = load_u64_le(chunk) ^ key64;
            store_u64_le(chunk, v);
        }
        i += 64;
    }

    while i + 32 <= data.len() {
        for chunk in data[i..i + 32].chunks_mut(8) {
            let v = load_u64_le(chunk) ^ key64;
            store_u64_le(chunk, v);
        }
        i += 32;
    }

    while i + 8 <= data.len() {
        let v = load_u64_le(&data[i..i + 8]) ^ key64;
        store_u64_le(&mut data[i..i + 8], v);
        i += 8;
    }

    while i + 4 <= data.len() {
        let v = load_u32_le(&data[i..i + 4]) ^ key32;
        store_u32_le(&mut data[i..i + 4], v);
        i += 4;
    }

    while i < data.len() {
        data[i] ^= key[i & 3];
        i += 1;
    }
}

/// Copy `size` bytes and unmask into `dst` (same layout as C++ `copy_and_unmask`).
pub fn copy_and_unmask(dst: &mut [u8], src: &[u8], key: &[u8; 4], dst_offset: usize) {
    if src.is_empty() {
        return;
    }

    let key32 = load_u32_le(key);
    let key64 = mask_key64(key32);

    let mut i = 0usize;
    let mut pos = dst_offset;

    while i < src.len() {
        if (pos & 7) == 0 && i + 8 <= src.len() {
            let v = load_u64_le(&src[i..i + 8]) ^ key64;
            store_u64_le(&mut dst[i..i + 8], v);
            i += 8;
            pos += 8;
            continue;
        }
        if (pos & 3) == 0 && i + 4 <= src.len() {
            let v = load_u32_le(&src[i..i + 4]) ^ key32;
            store_u32_le(&mut dst[i..i + 4], v);
            i += 4;
            pos += 4;
            continue;
        }
        dst[i] = src[i] ^ key[pos & 3];
        i += 1;
        pos += 1;
    }
}

pub fn generate_masking_key() -> [u8; 4] {
    use rand::Rng;
    let mut rng = rand::thread_rng();
    [rng.gen(), rng.gen(), rng.gen(), rng.gen()]
}
