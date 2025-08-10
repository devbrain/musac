#!/bin/bash

# Standalone build script that downloads NDK if needed
set -e

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
NDK_VERSION="r26b"
NDK_DIR="$SCRIPT_DIR/android-ndk-$NDK_VERSION"

# Check if NDK exists locally
if [ ! -d "$NDK_DIR" ]; then
    echo "Android NDK $NDK_VERSION not found locally."
    echo "Please download it from:"
    echo "https://developer.android.com/ndk/downloads"
    echo ""
    echo "For Linux, run:"
    echo "wget https://dl.google.com/android/repository/android-ndk-${NDK_VERSION}-linux.zip"
    echo "unzip android-ndk-${NDK_VERSION}-linux.zip -d $SCRIPT_DIR"
    echo ""
    echo "Then run this script again."
    exit 1
fi

export ANDROID_NDK_HOME="$NDK_DIR"
exec "$SCRIPT_DIR/build_android.sh"