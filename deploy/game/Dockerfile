# Build stage
FROM gcr.io/emubench-459802/emubench-dolphin-base:latest as builder

ARG GAME_ID
ARG GAME_PLATFORM

# Install Google Cloud SDK
RUN apt-get update && apt-get install -y curl gnupg2 software-properties-common
RUN curl https://packages.cloud.google.com/apt/doc/apt-key.gpg | gpg --dearmor -o /usr/share/keyrings/cloud.google.gpg
RUN echo "deb [signed-by=/usr/share/keyrings/cloud.google.gpg] https://packages.cloud.google.com/apt cloud-sdk main" | tee -a /etc/apt/sources.list.d/google-cloud-sdk.list
RUN apt-get update && apt-get install -y google-cloud-cli
COPY builder-key.json /tmp/builder-key.json
RUN gcloud auth activate-service-account --key-file=/tmp/builder-key.json

# Create destination directories
RUN mkdir -p /tmp/games /tmp/savestates

RUN gsutil cp gs://emubench-games/${GAME_PLATFORM}/${GAME_ID}.rvz /tmp/games/
RUN gsutil cp gs://emubench-savestates/${GAME_PLATFORM}/${GAME_ID}/* /tmp/savestates/ || true

# Final stage
FROM gcr.io/emubench-459802/emubench-dolphin-base:latest
COPY --from=builder /tmp/games/ /app/games/
COPY --from=builder /tmp/savestates/ /app/savestates/

ENV APP_MODE=production
