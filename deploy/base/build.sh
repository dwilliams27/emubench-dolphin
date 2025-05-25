#!/bin/bash

if [ $# -ne 1 ]; then
    echo "Usage: $0 <tag>"
    exit 1
fi

docker build -t gcr.io/emubench-459802/emubench-dolphin-base:$1 -f ./deploy/base/Dockerfile . && \
docker tag gcr.io/emubench-459802/emubench-dolphin-base:$1 gcr.io/emubench-459802/emubench-dolphin-base:latest && \
docker push gcr.io/emubench-459802/emubench-dolphin-base:$1 && \
docker push gcr.io/emubench-459802/emubench-dolphin-base:latest
