//! SemVer for **ws-rs** (from `rust/VERSION` / `CARGO_PKG_VERSION`).

/// Full version string (e.g. `"0.2.0"`).
pub const VERSION: &str = env!("CARGO_PKG_VERSION");

/// Major version component.
pub fn major() -> u32 {
    parse_component(0)
}

/// Minor version component.
pub fn minor() -> u32 {
    parse_component(1)
}

/// Patch version component.
pub fn patch() -> u32 {
    parse_component(2)
}

fn parse_component(index: usize) -> u32 {
    VERSION
        .split('.')
        .nth(index)
        .and_then(|s| s.parse().ok())
        .unwrap_or(0)
}
