#!/bin/bash
# Update any submodules recursively
git submodule update --init --recursive

# Extract the newest NDK path from our docker image
echo -n $(ls -d /ndk/* | sort -r | head -n 1) > ndkpath.txt

# Set qpm to use the same path as our docker image
qpm config ndk-path /ndk/

# Attempt to resolve the NDK specified in qpm.json
qpm ndk resolve -d || true

# Restore the dependencies
qpm restore
