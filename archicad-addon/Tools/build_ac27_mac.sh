#!/bin/bash
# Build the Tapir Add-On for Archicad 27 (macOS, universal RelWithDebInfo).
# Autonomous: downloads the DevKit if missing, configures CMake if needed, builds.
# Prints the path of the resulting .bundle on the last line.
set -euo pipefail

AC_VERSION=27
DEVKIT_URL="https://github.com/GRAPHISOFT/archicad-api-devkit/releases/download/27.3001/API.Development.Kit.MAC.27.3001.zip"

# Repo paths (this script lives in archicad-addon/Tools/).
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ADDON_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
cd "$ADDON_DIR"

DEVKIT_DIR="$ADDON_DIR/Build/DevKits/AC${AC_VERSION}"
BUILD_DIR="$ADDON_DIR/Build/AC${AC_VERSION}"
BUNDLE="$BUILD_DIR/RelWithDebInfo/TapirAddOn_AC${AC_VERSION}_Mac.bundle"

# Locate CMake (>= 3.17). Prefer PATH, fall back to the CMake.app bundle.
if command -v cmake >/dev/null 2>&1; then
    CMAKE=cmake
elif [ -x /Applications/CMake.app/Contents/bin/cmake ]; then
    CMAKE=/Applications/CMake.app/Contents/bin/cmake
else
    echo "error: cmake not found (install it or put CMake.app in /Applications)" >&2
    exit 1
fi

# 1. DevKit -------------------------------------------------------------------
if [ ! -d "$DEVKIT_DIR/Support" ]; then
    echo ">> Downloading Archicad ${AC_VERSION} DevKit..."
    rm -rf "$DEVKIT_DIR"
    mkdir -p "$DEVKIT_DIR"
    # curl is used instead of the Python helper because the system python3 on
    # this machine fails TLS verification against the GitHub release CDN.
    curl -fSL --retry 3 -o "$DEVKIT_DIR/devkit.zip" "$DEVKIT_URL"
    unzip -qq "$DEVKIT_DIR/devkit.zip" -d "$DEVKIT_DIR"
    rm -f "$DEVKIT_DIR/devkit.zip"
fi

# 2. Configure --------------------------------------------------------------
if [ ! -f "$BUILD_DIR/CMakeCache.txt" ]; then
    echo ">> Configuring CMake (Xcode generator)..."
    "$CMAKE" -B "$BUILD_DIR" -G Xcode \
        -DAC_VERSION=${AC_VERSION} \
        -DAC_API_DEVKIT_DIR="$DEVKIT_DIR/Support" \
        "$ADDON_DIR"
fi

# 3. Build ----------------------------------------------------------------
echo ">> Building TapirAddOn_AC${AC_VERSION}_Mac (RelWithDebInfo)..."
"$CMAKE" --build "$BUILD_DIR" --config RelWithDebInfo

if [ ! -d "$BUNDLE" ]; then
    echo "error: build finished but bundle not found at $BUNDLE" >&2
    exit 1
fi

echo ">> Build OK."
echo "$BUNDLE"
