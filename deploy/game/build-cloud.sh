#!/bin/bash

# Check if we're in the deploy/game directory
if [[ "$(basename "$PWD")" != "game" ]]; then
    echo "Error: This script must be run from the deploy/game directory"
    echo "Current directory: $PWD"
    echo "Expected to be in: .../deploy/game"
    exit 1
fi

if [ $# -ne 2 ]; then
    echo "Usage: $0 <platform> <game_id>"
    exit 1
fi

PLATFORM=$1
GAMEID_LOWER=$(echo "$2" | tr '[:upper:]' '[:lower:]')
GAMEID_UPPER=$(echo "$2" | tr '[:lower:]' '[:upper:]')

PROJECT_ID="emubench-459802"

echo "Building game image using Google Cloud Build..."

# Submit build to Cloud Build
gcloud builds submit \
    --config cloudbuild-game.yaml \
    --substitutions=_PLATFORM=$PLATFORM,_GAMEID_LOWER=$GAMEID_LOWER,_GAMEID_UPPER=$GAMEID_UPPER \
    --project=$PROJECT_ID \
    .

echo "Done!"
