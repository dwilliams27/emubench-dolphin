#!/bin/bash
# deploy/build.sh

# Navigate to the repository root
cd "$(dirname "$0")/.."

# Build the Docker image
docker build -t dolphin-linux-emu -f deploy/Dockerfile .

# Optionally tag with a version (from git or manually specified)
if [ -n "$1" ]; then
    # Use provided version tag
    VERSION="$1"
else
    # Use git hash as version
    VERSION=$(git rev-parse --short HEAD)
fi

# Tag the image
docker tag dolphin-linux-emu dolphin-linux-emu:$VERSION

echo "Built dolphin-linux-emu:$VERSION"
echo ""
echo "To run the emulator:"
echo "docker run --rm -it -e DISPLAY=host.docker.internal:0 dolphin-linux-emu:$VERSION"
echo ""
echo "To push to a container registry:"
echo "docker tag dolphin-linux-emu:$VERSION your-registry/dolphin-linux-emu:$VERSION"
echo "docker push your-registry/dolphin-linux-emu:$VERSION"
