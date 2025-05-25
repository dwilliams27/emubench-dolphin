# Build stage
FROM gcr.io/emubench-459802/emubench-dolphin-base:latest as builder

ARG GAME_ID
ARG GAME_PLATFORM

RUN apt-get update && apt-get install -y google-cloud-sdk
COPY deploy/game/builder-key.json /tmp/builder-key.json
RUN gcloud auth activate-service-account --key-file=/tmp/builder-key.json

RUN gsutil cp gs://emubench-games/${GAME_PLATFORM}/${GAME_ID}.rvz /tmp/games/
RUN gsutil cp gs://emubench-savestates/${GAME_PLATFORM}/${GAME_ID}/* /tmp/savestates/

# Final stage
FROM gcr.io/emubench-459802/emubench-dolphin-base:latest
COPY --from=builder /tmp/games/ /app/games/
COPY --from=builder /tmp/savestates/ /app/savestates/
ENV APP_MODE=production
