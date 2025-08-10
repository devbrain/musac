#!/bin/bash

# Build script for Musac WASM module
# Requires Emscripten SDK (emsdk) installed and activated

set -e

# Check if emcc is available
if ! command -v emcc &> /dev/null; then
    echo "Error: Emscripten (emcc) not found!"
    echo "Please install and activate emsdk:"
    echo "  git clone https://github.com/emscripten-core/emsdk.git"
    echo "  cd emsdk"
    echo "  ./emsdk install latest"
    echo "  ./emsdk activate latest"
    echo "  source ./emsdk_env.sh"
    exit 1
fi

echo "Emscripten version:"
emcc --version

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
BUILD_DIR="$SCRIPT_DIR/build"

# Clean and create build directory
rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"

# Build options
BUILD_TYPE="Release"
if [ "$1" == "debug" ]; then
    BUILD_TYPE="Debug"
    echo "Building in Debug mode..."
else
    echo "Building in Release mode (use './build_wasm.sh debug' for debug build)"
fi

# Configure with Emscripten
cd "$BUILD_DIR"
emcmake cmake .. \
    -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
    -DCMAKE_CXX_FLAGS="-fno-exceptions -fno-rtti"

# Build
emmake make -j$(nproc)

# Check output size
echo ""
echo "Build complete! Output files:"
ls -lh musac_wasm.* | awk '{print $9 ": " $5}'

# Optimize WASM file if in release mode
if [ "$BUILD_TYPE" == "Release" ] && command -v wasm-opt &> /dev/null; then
    echo ""
    echo "Optimizing WASM with wasm-opt..."
    wasm-opt -Oz musac_wasm.wasm -o musac_wasm_opt.wasm
    echo "Original size: $(ls -lh musac_wasm.wasm | awk '{print $5}')"
    echo "Optimized size: $(ls -lh musac_wasm_opt.wasm | awk '{print $5}')"
    mv musac_wasm_opt.wasm musac_wasm.wasm
fi

# Create distribution package
echo ""
echo "Creating distribution package..."
mkdir -p dist
cp musac_wasm.js dist/
cp musac_wasm.wasm dist/
cp ../src/musac.js dist/

# Add package.json for npm distribution
cat > dist/package.json << 'EOF'
{
  "name": "musac-wasm",
  "version": "1.0.0",
  "description": "Musac audio decoder library for web browsers",
  "main": "musac.js",
  "module": "musac.js",
  "files": [
    "musac.js",
    "musac_wasm.js",
    "musac_wasm.wasm"
  ],
  "keywords": [
    "audio",
    "decoder",
    "wasm",
    "mp3",
    "ogg",
    "flac",
    "wav",
    "webassembly"
  ],
  "author": "Musac Contributors",
  "license": "MIT"
}
EOF

echo ""
echo "Distribution package created in: $BUILD_DIR/dist/"
echo ""
echo "To use in a web page:"
echo '  <script type="module">'
echo '    import { MusacDecoder } from "./dist/musac.js";'
echo '    await MusacDecoder.initialize();'
echo '    // Now you can use MusacDecoder'
echo '  </script>'