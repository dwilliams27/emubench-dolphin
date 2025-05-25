#!/bin/bash

if [ $# -ne 2 ]; then
    echo "Usage: $0 <platform> <game_id>"
    exit 1
fi

GAME_PLATFORM="$1"
GAME_ID="$2"
GAME_ID_LOWER=$(echo "$GAME_ID" | tr '[:upper:]' '[:lower:]')
GAME_ID_UPPER=$(echo "$GAME_ID" | tr '[:lower:]' '[:upper:]')

docker build -t gcr.io/emubench-459802/emubench-$GAME_PLATFORM-$GAME_ID_LOWER:latest --build-arg GAME_ID=$GAME_ID_UPPER --build-arg GAME_PLATFORM=$GAME_PLATFORM -f ./deploy/game/Dockerfile . && \
docker push gcr.io/emubench-459802/emubench-$GAME_PLATFORM-$GAME_ID_LOWER:latest
