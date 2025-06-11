#!/bin/bash

if [ $# -ne 1 ]; then
    echo "Usage: $0 <tag>"
    exit 1
fi

TAG=$1
PROJECT_ID="emubench-459802"

echo "Building image using Google Cloud Build..."

# Submit build to Cloud Build
gcloud builds submit \
    --config cloudbuild.yaml \
    --substitutions=_TAG=$TAG \
    --project=$PROJECT_ID \
    .

echo "Build submitted to Cloud Build. Check the GCP Console for build status."
