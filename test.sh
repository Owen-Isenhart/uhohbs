#!/bin/bash
set -euo pipefail

NEW_NAME="uhohbs"
BUILD_DIR="build_x86_64"
SOURCE_SO="$BUILD_DIR/uhohbs.so"
DEST="$HOME/.config/obs-studio/plugins/$NEW_NAME/bin/64bit"
LOCALE_DEST="$HOME/.config/obs-studio/plugins/$NEW_NAME/data/locale"
LAUNCH_OBS="${1:-yes}"

cmake --preset ubuntu-x86_64
cmake --build --preset ubuntu-x86_64

mkdir -p "$DEST"
mkdir -p "$LOCALE_DEST"

if [ -f "$SOURCE_SO" ]; then
    cp "$SOURCE_SO" "$DEST/"
else
    echo "Primary path not found, searching for $NEW_NAME.so..."
    MATCH="$(find "$BUILD_DIR" -name "$NEW_NAME.so" | head -n 1)"
    if [ -z "$MATCH" ]; then
        echo "Could not find built plugin artifact ($NEW_NAME.so)."
        exit 1
    fi
    cp "$MATCH" "$DEST/"
fi

if [ ! -f "$DEST/$NEW_NAME.so" ]; then
    echo "Plugin deployment failed: $DEST/$NEW_NAME.so not found"
    exit 1
fi

if [ -f "data/locale/en-US.ini" ]; then
    cp "data/locale/en-US.ini" "$LOCALE_DEST/en-US.ini"
    echo "Locale deployed to: $LOCALE_DEST/en-US.ini"
else
    echo "Warning: data/locale/en-US.ini not found; UI text may show localization keys."
fi

echo "Plugin deployed to: $DEST/$NEW_NAME.so"

if [ "$LAUNCH_OBS" = "yes" ]; then
    echo "Launching OBS... look for [$NEW_NAME] in logs."
    /usr/local/bin/obs --verbose
else
    echo "Skipping OBS launch (pass 'yes' to launch)."
fi