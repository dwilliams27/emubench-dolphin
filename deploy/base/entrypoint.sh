#!/bin/bash
set -e

# Download save state if specified
if [ -n "$SAVE_STATE_PATH" ]; then
    echo "Downloading save state from: $SAVE_STATE_PATH"
    gsutil cp "$SAVE_STATE_PATH" /root/.dolphin-emu/StateSaves/
    
    # Extract filename for Dolphin
    SAVE_STATE_FILE=$(basename "$SAVE_STATE_PATH")
    echo "Downloaded save state: $SAVE_STATE_FILE"
fi

# For local testing
if [ -n "$SAVE_STATE_LOCAL_PATH" ]; then
    SAVE_STATE_FILE=$SAVE_STATE_LOCAL_PATH
    echo "Using local save state: $SAVE_STATE_FILE"
fi

# Auto-detect game file if not specified
if [ -z "$GAME_FILE" ]; then
    GAME_FILE=$(find /games -name "*.rvz" -o -name "*.iso" -o -name "*.wbfs" -o -name "*.gcm" | head -1)
fi

if [ -z "$GAME_FILE" ]; then
    echo "No game file found in /games/"
    exit 1
fi

echo "Running game: $GAME_FILE"

Xvfb :99 -screen 0 1280x720x24 & export DISPLAY=:99
exec /app/dolphin-emu-nogui -e $GAME_FILE --save_state $SAVE_STATE_FILE "$@"
