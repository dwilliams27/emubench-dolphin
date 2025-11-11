#!/bin/bash
set -e

# For local testing
if [ -n "$SAVE_STATE_LOCAL_PATH" ]; then
    SAVE_STATE_FILE=$SAVE_STATE_LOCAL_PATH
    echo "Using local save state: $SAVE_STATE_FILE"
fi

if [ -n "$SAVE_STATE_FILE" ]; then
    SAVE_STATE_FILE=/app/savestates/$SAVE_STATE_FILE
fi

if [ -z "$SAVE_STATE_FILE" ]; then
    SAVE_STATE_FILE=$(find /app/savestates -name "*.sav" | head -1)
fi

if [ -z "$SAVE_STATE_FILE" ]; then
    echo "No save states found in /app/savestates/"
    exit 1
fi

# Auto-detect game file if not specified
if [ -z "$GAME_FILE" ]; then
    GAME_FILE=$(find /app/games -name "*.rvz" -o -name "*.iso" -o -name "*.wbfs" -o -name "*.gcm" | head -1)
fi

if [ -z "$GAME_FILE" ]; then
    echo "No game file found in /app/games/"
    exit 1
fi

echo "Running game: $GAME_FILE"

TIMEOUT_MINUTES=30
echo "Container will auto-terminate after $TIMEOUT_MINUTES minutes"

Xvfb :99 -screen 0 1280x720x24 & export DISPLAY=:99
exec timeout ${TIMEOUT_MINUTES}m /app/dolphin-emu-nogui -e $GAME_FILE --save_state $SAVE_STATE_FILE --config Logger.Options.WriteToFile=true "$@"
