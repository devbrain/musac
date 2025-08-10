#!/bin/bash

# Build script for Musac Java bindings JAR

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"
CLASSES_DIR="$BUILD_DIR/classes"
JAR_FILE="$SCRIPT_DIR/core/build/libs/musac-core-1.0.0.jar"

echo "Building Musac Java bindings..."

# Create directories
mkdir -p "$CLASSES_DIR"
mkdir -p "$(dirname "$JAR_FILE")"

# Find all Java sources
echo "Finding Java sources..."
JAVA_FILES=$(find "$SCRIPT_DIR/src" -name "*.java")

# Compile Java sources
echo "Compiling Java sources..."
javac -d "$CLASSES_DIR" -source 1.8 -target 1.8 $JAVA_FILES

# Create JAR
echo "Creating JAR file..."
jar cf "$JAR_FILE" -C "$CLASSES_DIR" .

echo "Successfully built: $JAR_FILE"
echo "JAR size: $(du -h "$JAR_FILE" | cut -f1)"

# List contents
echo "JAR contents:"
jar tf "$JAR_FILE" | head -20
echo "..."