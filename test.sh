#!/bin/bash
NEW_NAME="uhohbs"
BUILD_DIR="build_x86_64"
SOURCE_SO="$BUILD_DIR/uhohbs.so" 
DEST="$HOME/.config/obs-studio/plugins/$NEW_NAME/bin/64bit"

cmake --preset ubuntu-x86_64
cmake --build --preset ubuntu-x86_64

if [ $? -ne 0 ]; then
    echo "Build failed! Check the errors above."
    exit 1
fi

rm -rf "$HOME/.config/obs-studio/plugins/plugintemplate-for-obs" 

mkdir -p "$DEST"

if [ -f "$SOURCE_SO" ]; then
    cp "$SOURCE_SO" "$DEST/"
else
    echo "Primary path not found, searching for $NEW_NAME.so..."
    find "$BUILD_DIR" -name "$NEW_NAME.so" -exec cp {} "$DEST/" \;
fi

echo "Launching OBS... look for [$NEW_NAME] in logs."
/usr/local/bin/obs --verbose