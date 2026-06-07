use ws_rs::version;

#[test]
fn version_string_non_empty() {
    assert!(!version::VERSION.is_empty());
}

#[test]
fn version_components_match_version_file() {
    // Synchronized with rust/VERSION (0.4.0)
    assert_eq!(version::major(), 0);
    assert_eq!(version::minor(), 4);
    assert_eq!(version::patch(), 0);
    assert_eq!(version::VERSION, "0.4.0");
}
