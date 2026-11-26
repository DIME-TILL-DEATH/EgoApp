#!/bin/bash
# Usage: check_bundle.sh <Bundle.app>
#
# Read-only checks run right after macdeployqt. NOTHING is removed or modified.
#   1. every @rpath/QtXxx.framework referenced by any Mach-O in the bundle
#      (main exe, wavconverter, frameworks, PlugIns, QML plugins) exists in
#      Contents/Frameworks  -> catches "Library not loaded" crashes at launch
#   2. the QML modules imported by EgoGIG and WavConverter are in Resources/qml
#      -> catches macdeployqt not scanning the right -qmldir
# Also prints the total size and the largest items.
# Compatible with the stock macOS bash 3.2.
set -u

APP="${1:-}"
if [ -z "$APP" ] || [ ! -d "$APP/Contents" ]; then
    echo "usage: $0 <Bundle.app>" >&2
    exit 2
fi
C="$APP/Contents"
missing=0

while IFS= read -r f; do
    case "$f" in
        *.qml|*.png|*.svg|*.pdf|*.qm|*.json|*.plist|*.icns|*.js|*.txt|*.metainfo|*.qmltypes|*.mjs) continue ;;
    esac
    file -b "$f" | grep -q 'Mach-O' || continue
    for fw in $(otool -L "$f" 2>/dev/null \
                | sed -n 's#^[[:space:]]*@rpath/\(Qt[A-Za-z0-9]*\)\.framework/.*#\1#p' | sort -u); do
        if [ ! -d "$C/Frameworks/$fw.framework" ]; then
            echo "MISSING: $f needs $fw.framework which is not in the bundle" >&2
            missing=1
        fi
    done
done < <(find "$APP" -type f)

# QML modules imported by EgoGIG or by WavConverter (both run from this bundle).
REQUIRED_QML="QtQuick/Controls/Basic QtQuick/Controls/Fusion QtQuick/Dialogs QtQuick/Layouts Qt/labs/platform"
for d in $REQUIRED_QML; do
    if [ ! -d "$C/Resources/qml/$d" ]; then
        echo "MISSING: QML module $d is not in $C/Resources/qml" >&2
        missing=1
    fi
done

echo "check_bundle: $APP: $(( $(du -sk "$APP" | awk '{print $1}') / 1024 )) MB"
echo "check_bundle: largest items:"
du -sk "$C"/Frameworks/*.framework "$C"/PlugIns/* "$C"/Resources/qml/* "$C"/MacOS/* 2>/dev/null \
    | sort -rn | head -12 | awk '{printf "  %7.1f MB  %s\n", $1/1024, $2}'

if [ "$missing" -ne 0 ]; then
    echo "check_bundle: FAILED - see MISSING above." >&2
    exit 1
fi
echo "check_bundle: OK"
