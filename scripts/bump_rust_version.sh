#!/bin/bash
#
# bump_rust_version.sh - SemVer bump for ws-rs (Rust workspace)
# Usage: ./scripts/bump_rust_version.sh [major|minor|patch|X.Y.Z] [--dry-run]
#

set -e

VERSION_FILE="rust/VERSION"
CARGO_FILE="rust/Cargo.toml"
DRY_RUN=0

for arg in "$@"; do
    if [ "$arg" = "--dry-run" ]; then
        DRY_RUN=1
    fi
done

show_help() {
    echo "Usage: $0 [major|minor|patch|X.Y.Z] [--dry-run]"
    echo ""
    echo "Updates rust/VERSION and [workspace.package] version in rust/Cargo.toml."
    echo "ws-rs versioning is independent from wscpp (root VERSION file)."
}

if [ ! -f "$VERSION_FILE" ]; then
    echo "Error: $VERSION_FILE not found"
    exit 1
fi

CURRENT_VERSION=$(tr -d '[:space:]' < "$VERSION_FILE")

if ! echo "$CURRENT_VERSION" | grep -qE '^[0-9]+\.[0-9]+\.[0-9]+$'; then
    echo "Error: invalid version in $VERSION_FILE (expected MAJOR.MINOR.PATCH)"
    exit 1
fi

MAJOR=$(echo "$CURRENT_VERSION" | cut -d. -f1)
MINOR=$(echo "$CURRENT_VERSION" | cut -d. -f2)
PATCH=$(echo "$CURRENT_VERSION" | cut -d. -f3)

BUMP_CMD=""
for arg in "$@"; do
    if [ "$arg" != "--dry-run" ]; then
        BUMP_CMD="$arg"
    fi
done

if [ -z "$BUMP_CMD" ]; then
    show_help
    exit 0
fi

case "$BUMP_CMD" in
    major)
        MAJOR=$((MAJOR + 1))
        NEW_VERSION="${MAJOR}.0.0"
        ;;
    minor)
        MINOR=$((MINOR + 1))
        NEW_VERSION="${MAJOR}.${MINOR}.0"
        ;;
    patch)
        PATCH=$((PATCH + 1))
        NEW_VERSION="${MAJOR}.${MINOR}.${PATCH}"
        ;;
    [0-9]*.[0-9]*.[0-9]*)
        if ! echo "$BUMP_CMD" | grep -qE '^[0-9]+\.[0-9]+\.[0-9]+$'; then
            echo "Error: invalid version format (expected MAJOR.MINOR.PATCH)"
            exit 1
        fi
        NEW_VERSION="$BUMP_CMD"
        ;;
    *)
        echo "Error: unknown command: $BUMP_CMD"
        show_help
        exit 1
        ;;
esac

if [ "$DRY_RUN" -eq 1 ]; then
    echo "Dry run: $CURRENT_VERSION -> $NEW_VERSION"
    echo "Would update: $VERSION_FILE"
    echo "Would update: $CARGO_FILE ([workspace.package] version)"
    exit 0
fi

echo "$NEW_VERSION" > "$VERSION_FILE"

if [ -f "$CARGO_FILE" ]; then
    sed -i "s/^version = \"[0-9][0-9]*\\.[0-9][0-9]*\\.[0-9][0-9]*\"/version = \"${NEW_VERSION}\"/" "$CARGO_FILE"
fi

echo "ws-rs version bumped: $CURRENT_VERSION -> $NEW_VERSION"
echo "Verify: cd rust && cargo test -p ws-rs --test version"
