#!/bin/bash
# Usage: prune_bundle.sh <Bundle.app>
#
# macdeployqt (with -qmldir) copies every Qt Quick Controls style and a number
# of unrelated modules (Qt3D, VirtualKeyboard, Timeline, ...) into the bundle.
# This script removes the ones EgoGIG / WavConverter never use, then verifies
# that no remaining Mach-O still links against a Qt framework that was removed.
#
# Edit the lists below if the app starts using one of the removed modules.
# Compatible with the stock macOS bash 3.2.
set -u

APP="${1:-}"
if [ -z "$APP" ] || [ ! -d "$APP/Contents" ]; then
    echo "usage: $0 <Bundle.app>" >&2
    exit 2
fi
C="$APP/Contents"

# Frameworks (without the .framework suffix) that nothing in the app needs.
FRAMEWORKS="
Qt3DAnimation Qt3DCore Qt3DExtras Qt3DInput Qt3DLogic Qt3DQuick
Qt3DQuickScene2D Qt3DQuickScene3D Qt3DRender
QtVirtualKeyboard QtVirtualKeyboardQml QtVirtualKeyboardSettings
QtQuickTimeline QtQuickTimelineBlendTrees QtQuickParticles
QtQuickShapesDesignHelpers
QtQuickVectorImage QtQuickVectorImageGenerator QtQuickVectorImageHelpers
QtShaderTools QtStateMachine QtStateMachineQml
QtSql QtQmlXmlListModel QtQmlLocalStorage
QtQuickControls2Material QtQuickControls2MaterialStyleImpl
QtQuickControls2Universal QtQuickControls2UniversalStyleImpl
QtQuickControls2Imagine QtQuickControls2ImagineStyleImpl
QtQuickControls2IOSStyleImpl QtQuickControls2FluentWinUI3StyleImpl
QtQuickControls2MacOSStyleImpl
"

# QML modules, relative to Contents/Resources/qml. The apps use Fusion (main)
# and Basic (converter); Basic stays as the fallback style of Fusion.
QML_DIRS="
Qt3D
QtQuick/VirtualKeyboard QtQuick/Timeline QtQuick/Particles
QtQuick/VectorImage QtQuick/LocalStorage
QtQuick/Controls/Material QtQuick/Controls/Universal QtQuick/Controls/Imagine
QtQuick/Controls/iOS QtQuick/Controls/FluentWinUI3 QtQuick/Controls/macOS
QtQml/StateMachine QtQml/XmlListModel
"

# Qt plugin categories, relative to Contents/PlugIns.
PLUGIN_DIRS="
qmltooling sqldrivers platforminputcontexts
renderers sceneparsers geometryloaders renderplugins
"

size_kb() { du -sk "$1" | awk '{print $1}'; }
before=$(size_kb "$APP")

removed=0
for f in $FRAMEWORKS; do
    if [ -d "$C/Frameworks/$f.framework" ]; then
        rm -rf "$C/Frameworks/$f.framework"; removed=$((removed + 1))
    fi
done
for d in $QML_DIRS; do
    if [ -d "$C/Resources/qml/$d" ]; then
        rm -rf "$C/Resources/qml/$d"; removed=$((removed + 1))
    fi
done
for d in $PLUGIN_DIRS; do
    if [ -d "$C/PlugIns/$d" ]; then
        rm -rf "$C/PlugIns/$d"; removed=$((removed + 1))
    fi
done

# Safety net: every @rpath/QtXxx.framework referenced by a remaining Mach-O
# must still exist in this bundle.
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
# If one is missing, macdeployqt did not scan the right -qmldir.
REQUIRED_QML="QtQuick/Controls/Basic QtQuick/Controls/Fusion QtQuick/Dialogs QtQuick/Layouts Qt/labs/platform"
for d in $REQUIRED_QML; do
    if [ ! -d "$C/Resources/qml/$d" ]; then
        echo "MISSING: QML module $d is not in $C/Resources/qml" >&2
        missing=1
    fi
done

after=$(size_kb "$APP")
echo "prune_bundle: $APP: removed $removed items, $((before / 1024)) MB -> $((after / 1024)) MB"
echo "prune_bundle: largest remaining items:"
du -sk "$C"/Frameworks/*.framework "$C"/PlugIns/* "$C"/Resources/qml/* 2>/dev/null \
    | sort -rn | head -12 | awk '{printf "  %7.1f MB  %s\n", $1/1024, $2}'

if [ "$missing" -ne 0 ]; then
    echo "prune_bundle: FAILED - a Qt framework or QML module the app needs is missing (see MISSING above)." >&2
    echo "              If it was removed by this script, take it out of the lists in $0." >&2
    exit 1
fi
