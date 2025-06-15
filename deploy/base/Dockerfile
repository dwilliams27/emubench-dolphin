FROM ubuntu:22.04 AS builder

ARG DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    git \
    pkg-config \
    gettext \
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
    ninja-build \
    ca-certificates && \
    apt-get clean && rm -rf /var/lib/apt/lists/*

WORKDIR /dolphin-src

COPY . .

RUN mkdir build && cd build && \
    cmake .. -G Ninja \
    -DLINUX_LOCAL_DEV=ON \
    -DCMAKE_BUILD_TYPE=Release \
    -DENABLE_GENERIC=ON \
    -DDISABLE_ADVANCE_SIMD=ON && \
    ninja

FROM ubuntu:22.04 AS runtime

ARG DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    # Essential for running GUI apps in a headless environment via entrypoint
    xvfb \
    # FFmpeg runtime libraries
    libavcodec58 libavformat58 libavutil56 libswscale5 \
    # X11 and related GUI libraries
    libxi6 libxrandr2 libxcursor1 libxinerama1 libxfixes3 libxrender1 libxkbcommon0 libxkbcommon-x11-0 libfontconfig1 libfreetype6 \
    # Input/Device libraries
    libudev1 libevdev2 \
    # SFML runtime libraries (Ubuntu 22.04 uses SFML 2.5.1)
    libsfml-audio2.5 libsfml-graphics2.5 libsfml-network2.5 libsfml-system2.5 libsfml-window2.5 \
    # Networking and Crypto libraries
    libminiupnpc17 libmbedtls14 libmbedcrypto7 libmbedx509-1 libcurl4 \
    # HIDAPI runtime
    libhidapi-hidraw0 \
    # Systemd runtime (often a dependency of other components)
    libsystemd0 \
    # Bluetooth runtime
    libbluetooth3 \
    # Audio runtime
    libasound2 libpulse0 \
    # Graphics runtime (OpenGL and Vulkan loaders)
    libgl1 libvulkan1 \
    # Qt6 runtime libraries
    libqt6core6 libqt6dbus6 libqt6gui6 libqt6network6 libqt6opengl6 libqt6widgets6 libqt6xml6 libqt6svg6 \
    # Qt6 platform plugins (essential for GUI)
    qt6-qpa-plugins \
    # Other common shared libraries that might be pulled in or needed by Qt
    libglib2.0-0 libpng16-16 zlib1g libicu70 \
    libsm6 libice6 \
    # For find, head, etc. in entrypoint (usually part of base)
    coreutils \
    # Ensure ca-certificates are present for HTTPS connections
    ca-certificates \
    && apt-get clean && rm -rf /var/lib/apt/lists/*

WORKDIR /app
RUN mkdir -p /root/.dolphin-emu/Config \
    && mkdir -p /app/games \
    && mkdir -p /app/savestates \
    && mkdir -p /app/emu

COPY --from=builder /dolphin-src/build/Binaries/dolphin-emu /app/dolphin-emu
COPY --from=builder /dolphin-src/build/Binaries/dolphin-emu-nogui /app/dolphin-emu-nogui
COPY --from=builder /dolphin-src/Data/Sys /app/Sys

COPY deploy/configs/Dolphin.ini /root/.dolphin-emu/Config/Dolphin.ini
COPY deploy/base/entrypoint.sh /app/entrypoint.sh

RUN chmod +x /app/entrypoint.sh

ENV DOLPHIN_EMU_USERPATH="/app/emu"

EXPOSE 8080

ENTRYPOINT ["/app/entrypoint.sh"]
CMD []
