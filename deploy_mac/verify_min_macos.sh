#!/bin/bash
# Usage: verify_min_macos.sh <Bundle.app> <min-macos, e.g. 11.0>
#
# Fails (exit 1) if the bundle, any nested .app, framework, plugin or dylib
# requires a macOS version newer than <min-macos>:
#   * LSMinimumSystemVersion in every *.app/Contents/Info.plist
#   * LC_BUILD_VERSION "minos" / LC_VERSION_MIN_MACOSX "version" of every Mach-O
# Compatible with the stock macOS bash 3.2.
set -u

APP="${1:-}"
MIN="${2:-}"
if [ -z "$APP" ] || [ -z "$MIN" ] || [ ! -d "$APP" ]; then
    echo "usage: $0 <Bundle.app> <min-macos>" >&2
    exit 2
fi

# ver_gt A B  -> exit 0 if version A is strictly greater than version B
ver_gt() {
    awk -v a="$1" -v b="$2" 'BEGIN {
        n = split(a, x, "."); m = split(b, y, ".");
        k = (n > m) ? n : m;
        for (i = 1; i <= k; i++) {
            xi = (x[i] == "") ? 0 : x[i] + 0;
            yi = (y[i] == "") ? 0 : y[i] + 0;
            if (xi > yi) exit 0;
            if (xi < yi) exit 1;
        }
        exit 1
    }'
}

fail=0
checked=0

# --- 1. Info.plist of the app and of nested apps ---------------------------
while IFS= read -r plist; do
    v=$(/usr/libexec/PlistBuddy -c "Print :LSMinimumSystemVersion" "$plist" 2>/dev/null)
    if [ -z "$v" ]; then
        echo "WARN: no LSMinimumSystemVersion in $plist"
    elif ver_gt "$v" "$MIN"; then
        echo "FAIL: LSMinimumSystemVersion=$v (> $MIN) in $plist"
        fail=1
    else
        echo "ok:   LSMinimumSystemVersion=$v in $plist"
    fi
done < <(find "$APP" -path '*.app/Contents/Info.plist')

# --- 2. Mach-O load commands ----------------------------------------------
while IFS= read -r f; do
    case "$f" in
        *.qml|*.png|*.svg|*.pdf|*.qm|*.json|*.plist|*.icns|*.js|*.txt|*.metainfo|*.qmltypes|*.mjs) continue ;;
    esac
    file -b "$f" | grep -q 'Mach-O' || continue
    checked=$((checked + 1))
    for v in $(otool -l "$f" 2>/dev/null | awk '
            /^Load command/                 { mode = "" }
            /cmd LC_BUILD_VERSION/          { mode = "b" }
            /cmd LC_VERSION_MIN_MACOSX/     { mode = "m" }
            mode == "b" && $1 == "minos"    { print $2 }
            mode == "m" && $1 == "version"  { print $2 }'); do
        if ver_gt "$v" "$MIN"; then
            echo "FAIL: requires macOS $v (> $MIN): $f"
            fail=1
        fi
    done
done < <(find "$APP" -type f)

echo "verify_min_macos: checked $checked Mach-O files against macOS $MIN"
if [ "$fail" -ne 0 ]; then
    echo "verify_min_macos: FAILED - the bundle would not run on macOS $MIN" >&2
    exit 1
fi
echo "verify_min_macos: OK"
