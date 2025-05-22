#!/bin/bash
set -e

# Auto-detect game file if not specified
if [ -z "$GAME_FILE" ]; then
    GAME_FILE=$(find /games -name "*.iso" -o -name "*.wbfs" -o -name "*.gcm" -o -name "*.rvz" | head -1)
fi

if [ -z "$GAME_FILE" ]; then
    echo "No game file found in /games/"
    exit 1
fi

echo "Running game: $GAME_FILE"
Xvfb :99 -screen 0 1280x720x24 & export DISPLAY=:99
exec /app/dolphin-emu-nogui "$GAME_FILE" "$@"
