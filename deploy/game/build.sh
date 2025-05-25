#!/bin/bash

if [ $# -ne 2 ]; then
    echo "Usage: $0 <platform> <game_id>"
    exit 1
fi

GAME_PLATFORM="$1"
GAME_ID="$2"

docker build -t gcr.io/emubench-459802/emubench-$GAME_PLATFORM-$GAME_ID:latest -f ./deploy/game/Dockerfile . 
docker push gcr.io/emubench-459802/emubench-$GAME_PLATFORM-$GAME_ID:latest
