#!/bin/bash

# Android NDK build script for musac_java library
# This script builds the native library for all Android ABIs

set -e

# Check if ANDROID_NDK_HOME is set
if [ -z "$ANDROID_NDK_HOME" ]; then
    # Try to find NDK in common locations
    if [ -d "$HOME/Android/Sdk/ndk" ]; then
        # Find the latest NDK version
        ANDROID_NDK_HOME=$(ls -d $HOME/Android/Sdk/ndk/* | sort -V | tail -1)
    elif [ -d "/opt/android-ndk" ]; then
        ANDROID_NDK_HOME="/opt/android-ndk"
    else
        echo "Error: ANDROID_NDK_HOME is not set and NDK not found in common locations"
        echo "Please set ANDROID_NDK_HOME to your Android NDK installation path"
        exit 1
    fi
fi

echo "Using Android NDK at: $ANDROID_NDK_HOME"

# Get the directory of this script
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
BUILD_DIR="$SCRIPT_DIR/build"
OUTPUT_DIR="$SCRIPT_DIR/src/main/jniLibs"

# Clean previous builds
rm -rf "$BUILD_DIR"
rm -rf "$OUTPUT_DIR"
mkdir -p "$OUTPUT_DIR"

# List of ABIs to build
ABIS=("armeabi-v7a" "arm64-v8a" "x86" "x86_64")

# Build for each ABI
for ABI in "${ABIS[@]}"; do
    echo "Building for $ABI..."
    
    BUILD_ABI_DIR="$BUILD_DIR/$ABI"
    mkdir -p "$BUILD_ABI_DIR"
    
    # Configure with CMake
    cmake -B "$BUILD_ABI_DIR" \
        -DCMAKE_TOOLCHAIN_FILE="$ANDROID_NDK_HOME/build/cmake/android.toolchain.cmake" \
        -DANDROID_ABI="$ABI" \
        -DANDROID_PLATFORM=android-21 \
        -DANDROID_STL=c++_shared \
        -DCMAKE_BUILD_TYPE=Release \
        -S "$SCRIPT_DIR"
    
    # Build
    cmake --build "$BUILD_ABI_DIR" --target musac_java -j$(nproc)
    
    # Copy the built library to jniLibs
    mkdir -p "$OUTPUT_DIR/$ABI"
    cp "$BUILD_ABI_DIR/libmusac_java.so" "$OUTPUT_DIR/$ABI/"
    
    # Also copy the C++ STL library
    STL_LIB="$ANDROID_NDK_HOME/toolchains/llvm/prebuilt/linux-x86_64/sysroot/usr/lib/$ABI/libc++_shared.so"
    if [ -f "$STL_LIB" ]; then
        cp "$STL_LIB" "$OUTPUT_DIR/$ABI/"
    fi
    
    echo "Built $ABI successfully"
done

echo "All ABIs built successfully!"
echo "Libraries are in: $OUTPUT_DIR"

# List the output
echo ""
echo "Generated libraries:"
find "$OUTPUT_DIR" -name "*.so" -type f | sort