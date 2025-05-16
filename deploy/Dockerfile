# deploy/Dockerfile
# Multi-stage build for Dolphin Emulator (Linux version)
FROM ubuntu:22.04 as builder

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    pkg-config \
    libavcodec-dev \
    libavformat-dev \
    libavutil-dev \
    libswscale-dev \
    libxi-dev \
    libxrandr-dev \
    libudev-dev \
    libevdev-dev \
    libsfml-dev \
    libminiupnpc-dev \
    libmbedtls-dev \
    libcurl4-openssl-dev \
    libhidapi-dev \
    libsystemd-dev \
    libbluetooth-dev \
    libasound2-dev \
    libpulse-dev \
    libgl1-mesa-dev \
    libvulkan-dev \
    qt6-base-dev \
    qt6-base-private-dev \
    libqt6svg6-dev \
    xvfb \
    xdg-utils \
    ninja-build

WORKDIR /dolphin-src

# This assumes the Dockerfile is run from the repository root
COPY . .

RUN mkdir build && cd build && \
    cmake .. -G Ninja -DLINUX_LOCAL_DEV=ON -DCMAKE_BUILD_TYPE=Release && \
    ninja

RUN mkdir -p /app

RUN cp build/Binaries/dolphin-emu /app/ && \
    cp build/Binaries/dolphin-emu-nogui /app/ && \
    mkdir -p /app/Sys && \
    cp -r Data/Sys/* /app/Sys/

RUN mkdir -p /root/.dolphin-emu/Config \
    && mkdir -p /games \
    && mkdir -p /output/screenshots \
    && mkdir -p /logs

COPY deploy/configs/Dolphin.ini /root/.dolphin-emu/Config/Dolphin.ini

COPY deploy/entrypoint.sh /app/
RUN chmod +x /app/entrypoint.sh
RUN chmod +x /dolphin-src/build/Binaries/dolphin-emu-nogui

WORKDIR /app
RUN rm -rf /dolphin-src

ENTRYPOINT ["/app/entrypoint.sh"]
