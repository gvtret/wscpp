#!/bin/bash
#
# bump_version.sh - SemVer bump for wscpp
# Usage: ./scripts/bump_version.sh [major|minor|patch|X.Y.Z] [--dry-run]
#

set -e

VERSION_FILE="VERSION"
CMAKE_FILE="CMakeLists.txt"
DRY_RUN=0

for arg in "$@"; do
    if [ "$arg" = "--dry-run" ]; then
        DRY_RUN=1
    fi
done

show_help() {
    echo "Usage: $0 [major|minor|patch|X.Y.Z] [--dry-run]"
    echo ""
    echo "Commands:"
    echo "  major      Bump major version (reset minor and patch)"
    echo "  minor      Bump minor version (reset patch)"
    echo "  patch      Bump patch version"
    echo "  X.Y.Z      Set explicit version"
    echo "  --dry-run  Show changes without writing files"
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
    echo "Would update: $CMAKE_FILE (project VERSION)"
    exit 0
fi

echo "$NEW_VERSION" > "$VERSION_FILE"

if [ -f "$CMAKE_FILE" ]; then
    sed -i "s/^    VERSION [0-9][0-9]*\\.[0-9][0-9]*\\.[0-9][0-9]*/    VERSION ${NEW_VERSION}/" "$CMAKE_FILE"
fi

echo "Version bumped: $CURRENT_VERSION -> $NEW_VERSION"
echo "Reconfigure: cmake -B build && cmake --build build"
