TMP_DIR="dmg_content"
APP_BUNDLE_NAME="EgoGIG.app"

DMG_NAME="EgoGIG.dmg"
DMG_NAME_TMP="${APP_BUNDLE_NAME%.*}_tmp.dmg"
VOL_NAME=${APP_BUNDLE_NAME%.*}

# cleanup previous files
rm -f "${DMG_NAME}"
rm -f "${DMG_NAME_TMP}"

hdiutil create -ov -srcfolder ${TMP_DIR} -format UDRW -volname "${VOL_NAME}" "${DMG_NAME_TMP}"
device=$(hdiutil attach -readwrite -noverify -noautoopen ${DMG_NAME_TMP} | egrep '^/dev/' | sed 1q | awk '{print $1}')

BG_IMG_NAME="background.png"
BG_FOLDER="/Volumes/${VOL_NAME}/.background"
mkdir "${BG_FOLDER}"
cp "${BG_IMG_NAME}" "${BG_FOLDER}/"

ICON_FOLDER="/Volumes/${VOL_NAME}/"
cp "VolumeIcon.icns" "${ICON_FOLDER}/.VolumeIcon.icns"

ARG_ICON_SIZE=160

BUNDLE_X=150
BUNDLE_Y=230

APPS_X=490
APPS_Y=220

WINDOW_LEFT=400
WINDOW_TOP=100
WINDOW_WIDTH=640
WINDOW_HEIGHT=480

WINDOW_RIGHT=$((${WINDOW_LEFT} + ${WINDOW_WIDTH}))
WINDOW_BOTTOM=$((${WINDOW_TOP} + ${WINDOW_HEIGHT}))

# customize image
APPLESCRIPT="
tell application \"Finder\"
        tell disk \"${VOL_NAME}\"
                open
                -- Setting view options
                set current view of container window to icon view
                set toolbar visible of container window to false
                set statusbar visible of container window to false
                set the bounds of container window to {${WINDOW_LEFT}, ${WINDOW_TOP}, ${WINDOW_RIGHT}, ${WINDOW_BOTTOM}}
                set theViewOptions to the icon view options of container window
                set arrangement of theViewOptions to not arranged
                set icon size of theViewOptions to ${ARG_ICON_SIZE}
                -- Settings background
                set background picture of theViewOptions to file \".background:${BG_IMG_NAME}\"
                -- Adding symlink to /Applications
                make new alias file at container window to POSIX file \"/Applications\" with properties {name:\"Applications\"}
                -- Reopening
                close
                open
                -- Rearranging
                set the position of item \"Applications\" to {${APPS_X}, ${APPS_Y}}
                set the position of item \"${APP_BUNDLE_NAME}\" to {${BUNDLE_X}, ${BUNDLE_Y}}
                -- Updating and sleeping for 5 secs
                update without registering applications
                delay 5
        end tell
end tell
"

echo "$APPLESCRIPT" | osascript
echo "AppleScript done"

echo "    * Setting volume icon... "
SetFile -c icnC "${ICON_FOLDER}/.VolumeIcon.icns"
SetFile -a C ${ICON_FOLDER}
echo "done!"

## on some step system remove icon. Copy again
cp "VolumeIcon.icns" "${ICON_FOLDER}/.VolumeIcon.icns"

# set rights and sync(2 times for reliable)
chmod -Rf go-w /Volumes/"${VOL_NAME}"
sync
sync
hdiutil detach ${device}

# finalize image
hdiutil convert "${DMG_NAME_TMP}" -format UDZO -imagekey zlib-level=9 -o "${DMG_NAME}"

echo "*** Removing temporary image... "
rm -f "${DMG_NAME_TMP}"
echo "done!"
