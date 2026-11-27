#!/bin/bash
# Usage: check_signing.sh <EgoGIG.app> <appstore|standalone>
#
# Sanity checks after codesign, so a wrongly signed helper is caught at build
# time and not by App Store review / notarization / a crash on the user's Mac.
#   appstore  : main exe sandboxed (no inherit); wavconverter has EXACTLY
#               app-sandbox + inherit (Apple allows nothing else together with inherit)
#   standalone: main exe and wavconverter have the hardened runtime flag
# Compatible with the stock macOS bash 3.2.
set -u

APP="${1:-}"
MODE="${2:-}"
HELPER="$APP/Contents/MacOS/wavconverter"
MAIN="$APP/Contents/MacOS/$(/usr/libexec/PlistBuddy -c 'Print :CFBundleExecutable' "$APP/Contents/Info.plist" 2>/dev/null)"
if [ ! -d "$APP" ] || [ ! -f "$HELPER" ] || { [ "$MODE" != appstore ] && [ "$MODE" != standalone ]; }; then
    echo "usage: $0 <EgoGIG.app> <appstore|standalone>  (and $HELPER must exist)" >&2
    exit 2
fi

fail=0
ents() { codesign -d --entitlements :- "$1" 2>/dev/null; }
bad()  { echo "FAIL: $*" >&2; fail=1; }

if [ "$MODE" = appstore ]; then
    ents "$MAIN"   | grep -q 'com.apple.security.app-sandbox' || bad "main executable is not sandboxed"
    ents "$MAIN"   | grep -q 'com.apple.security.inherit'     && bad "main executable must not have com.apple.security.inherit"
    ents "$HELPER" | grep -q 'com.apple.security.app-sandbox' || bad "wavconverter lacks com.apple.security.app-sandbox"
    ents "$HELPER" | grep -q 'com.apple.security.inherit'     || bad "wavconverter lacks com.apple.security.inherit (it would crash when started by EgoGIG)"
    n=$(ents "$HELPER" | grep -c '<key>')
    [ "$n" -eq 2 ] || bad "wavconverter has $n entitlements, expected exactly 2 (app-sandbox, inherit)"
else
    for f in "$MAIN" "$HELPER"; do
        codesign -dv "$f" 2>&1 | grep -q 'flags=.*runtime' || bad "hardened runtime is missing: $f"
    done
fi

if [ "$fail" -ne 0 ]; then
    echo "check_signing ($MODE): FAILED" >&2
    exit 1
fi
echo "check_signing ($MODE): OK"
