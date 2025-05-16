#!/bin/bash

# Ensure directories exist
mkdir -p /games
mkdir -p /output/screenshots
mkdir -p /logs

/app/dolphin-emu-nogui "$@"
