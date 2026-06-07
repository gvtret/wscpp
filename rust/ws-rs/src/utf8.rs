/// Validate UTF-8 bytes (RFC 6455 §8.1 — text frames must be valid UTF-8).
/// Ported from `wscpp/detail/utf8.hpp` (ASCII fast path + strict multibyte checks).
pub fn is_valid_utf8(data: &[u8]) -> bool {
    let mut i = 0usize;
    let len = data.len();

    while i + 8 <= len {
        let chunk = u64::from_le_bytes(data[i..i + 8].try_into().unwrap());
        if (chunk & 0x8080_8080_8080_8080) != 0 {
            break;
        }
        i += 8;
    }

    while i < len {
        let c = data[i];
        if c <= 0x7F {
            i += 1;
            continue;
        }
        if (0xC2..=0xDF).contains(&c) {
            if i + 1 >= len || data[i + 1] & 0xC0 != 0x80 {
                return false;
            }
            i += 2;
            continue;
        }
        if (0xE0..=0xEF).contains(&c) {
            if i + 2 >= len {
                return false;
            }
            let c1 = data[i + 1];
            let c2 = data[i + 2];
            if c1 & 0xC0 != 0x80 || c2 & 0xC0 != 0x80 {
                return false;
            }
            if c == 0xE0 && c1 < 0xA0 {
                return false;
            }
            if c == 0xED && c1 >= 0xA0 {
                return false;
            }
            i += 3;
            continue;
        }
        if (0xF0..=0xF4).contains(&c) {
            if i + 3 >= len {
                return false;
            }
            let c1 = data[i + 1];
            let c2 = data[i + 2];
            let c3 = data[i + 3];
            if c1 & 0xC0 != 0x80 || c2 & 0xC0 != 0x80 || c3 & 0xC0 != 0x80 {
                return false;
            }
            if c == 0xF0 && c1 < 0x90 {
                return false;
            }
            if c == 0xF4 && c1 >= 0x90 {
                return false;
            }
            i += 4;
            continue;
        }
        return false;
    }
    true
}
