#!/bin/bash
# Deploy the freshly built Tapir Add-On into Archicad 27's Extensions folder,
# overwriting the currently installed Tapir bundle.
#
# REFUSES to run while Archicad 27 is open -- quit it first.
set -euo pipefail

AC_VERSION=27
EXT_DIR="/Applications/Graphisoft/Archicad ${AC_VERSION}/Extensions"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ADDON_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
SRC_BUNDLE="$ADDON_DIR/Build/AC${AC_VERSION}/RelWithDebInfo/TapirAddOn_AC${AC_VERSION}_Mac.bundle"

# 1. Preconditions --------------------------------------------------------
if [ ! -d "$SRC_BUNDLE" ]; then
    echo "error: no built bundle at $SRC_BUNDLE -- run build_ac27_mac.sh first" >&2
    exit 1
fi
if [ ! -d "$EXT_DIR" ]; then
    echo "error: Extensions folder not found: $EXT_DIR" >&2
    exit 1
fi
# Match only an Archicad 27 process, by its actual executable path (via /proc-less ps,
# since macOS has no /proc). `pgrep -x Archicad` alone would also match a *different*
# installed version (e.g. Archicad 26) running at the same time - same process name,
# wrong app. `pgrep -f` is avoided too: it also matches unrelated shells whose command
# line happens to contain the Archicad path (e.g. this script's own invocation).
if pgrep -x Archicad >/dev/null 2>&1; then
    if ps -axo pid,command | grep "Archicad ${AC_VERSION}.app/Contents/MacOS/Archicad" | grep -v grep >/dev/null; then
        echo "error: Archicad ${AC_VERSION} is still running. Quit it and re-run." >&2
        exit 1
    fi
fi

# 2. Find the installed Tapir bundle (keep its exact name so Archicad's
#    Add-On Manager entry keeps pointing at it). Fall back to canonical name.
DEST_BUNDLE=""
while IFS= read -r -d '' b; do
    DEST_BUNDLE="$b"
    break
done < <(find "$EXT_DIR" -maxdepth 1 -iname "*tapir*AC${AC_VERSION}*.bundle" -print0)

if [ -z "$DEST_BUNDLE" ]; then
    DEST_BUNDLE="$EXT_DIR/TapirAddOn_AC${AC_VERSION}_Mac.bundle"
    echo ">> No existing Tapir bundle found; installing as: $DEST_BUNDLE"
else
    echo ">> Overwriting installed bundle: $DEST_BUNDLE"
fi

# 3. Replace atomically-ish: copy alongside, then swap.
STAGING="${DEST_BUNDLE}.new-$$"
rm -rf "$STAGING"
# ditto preserves bundle symlinks and permission bits.
ditto "$SRC_BUNDLE" "$STAGING"
rm -rf "$DEST_BUNDLE"
mv "$STAGING" "$DEST_BUNDLE"

echo ">> Done. Installed:"
/usr/bin/find "$DEST_BUNDLE" -name Info.plist -maxdepth 2 -exec /usr/libexec/PlistBuddy -c 'Print :CFBundleShortVersionString' {} \; 2>/dev/null || true
echo "$DEST_BUNDLE"
