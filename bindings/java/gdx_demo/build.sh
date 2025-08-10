#!/bin/bash

# Musac libGDX Demo - Unified Build Script

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Default values
BUILD_DIR="build-cmake"
PLATFORMS=""
CMAKE_ARGS=""
BUILD_TYPE="Release"
VERBOSE=false

# Print usage
usage() {
    echo "Usage: $0 [OPTIONS] [PLATFORMS]"
    echo ""
    echo "PLATFORMS:"
    echo "  desktop    Build desktop version (Windows/Linux/Mac)"
    echo "  android    Build Android version (APK and AAB)"
    echo "  web        Build Web/HTML5 version"
    echo "  ios        Build iOS version (macOS only)"
    echo "  all        Build all platforms"
    echo ""
    echo "OPTIONS:"
    echo "  -d, --debug       Build debug version"
    echo "  -c, --clean       Clean before building"
    echo "  -r, --run PLATFORM Run specified platform after build"
    echo "  -v, --verbose     Verbose output"
    echo "  -h, --help        Show this help"
    echo ""
    echo "EXAMPLES:"
    echo "  $0 desktop                  # Build desktop only"
    echo "  $0 desktop android           # Build desktop and Android"
    echo "  $0 all                       # Build all platforms"
    echo "  $0 -c -r desktop desktop     # Clean, build and run desktop"
    echo "  $0 -d android                # Build Android debug version"
    exit 0
}

# Parse arguments
RUN_PLATFORM=""
CLEAN=false

while [[ $# -gt 0 ]]; do
    case $1 in
        -d|--debug)
            BUILD_TYPE="Debug"
            shift
            ;;
        -c|--clean)
            CLEAN=true
            shift
            ;;
        -r|--run)
            RUN_PLATFORM="$2"
            shift 2
            ;;
        -v|--verbose)
            VERBOSE=true
            CMAKE_ARGS="$CMAKE_ARGS -DCMAKE_VERBOSE_MAKEFILE=ON"
            shift
            ;;
        -h|--help)
            usage
            ;;
        desktop|android|web|ios)
            PLATFORMS="$PLATFORMS $1"
            shift
            ;;
        all)
            PLATFORMS="desktop android web ios"
            shift
            ;;
        *)
            echo -e "${RED}Unknown option: $1${NC}"
            usage
            ;;
    esac
done

# If no platforms specified, show usage
if [ -z "$PLATFORMS" ]; then
    echo -e "${YELLOW}No platforms specified${NC}"
    usage
fi

# Clean if requested
if [ "$CLEAN" = true ]; then
    echo -e "${YELLOW}Cleaning previous builds...${NC}"
    rm -rf "$BUILD_DIR"
    ./gradlew clean 2>/dev/null || true
fi

# Create build directory
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Prepare CMake arguments
CMAKE_CONFIG_ARGS="-DCMAKE_BUILD_TYPE=$BUILD_TYPE"

# Configure platforms
for platform in $PLATFORMS; do
    case $platform in
        desktop)
            CMAKE_CONFIG_ARGS="$CMAKE_CONFIG_ARGS -DBUILD_DESKTOP=ON"
            ;;
        android)
            CMAKE_CONFIG_ARGS="$CMAKE_CONFIG_ARGS -DBUILD_ANDROID=ON"
            # Check Android SDK
            if [ -z "$ANDROID_SDK_ROOT" ] && [ -z "$ANDROID_HOME" ]; then
                echo -e "${RED}Error: ANDROID_SDK_ROOT or ANDROID_HOME must be set${NC}"
                exit 1
            fi
            ;;
        web)
            CMAKE_CONFIG_ARGS="$CMAKE_CONFIG_ARGS -DBUILD_WEB=ON"
            # Check Emscripten
            if [ -z "$EMSDK" ]; then
                echo -e "${RED}Error: EMSDK must be set. Please install and source Emscripten SDK${NC}"
                echo "Visit: https://emscripten.org/docs/getting_started/downloads.html"
                exit 1
            fi
            ;;
        ios)
            CMAKE_CONFIG_ARGS="$CMAKE_CONFIG_ARGS -DBUILD_IOS=ON"
            # Check if on macOS
            if [ "$(uname)" != "Darwin" ]; then
                echo -e "${RED}Error: iOS builds can only be performed on macOS${NC}"
                exit 1
            fi
            ;;
    esac
done

# Configure
echo -e "${GREEN}Configuring build...${NC}"
echo "Platforms: $PLATFORMS"
echo "Build type: $BUILD_TYPE"

if [ "$VERBOSE" = true ]; then
    cmake $CMAKE_CONFIG_ARGS $CMAKE_ARGS ..
else
    cmake $CMAKE_CONFIG_ARGS $CMAKE_ARGS .. > /dev/null 2>&1
fi

# Build each platform
for platform in $PLATFORMS; do
    echo -e "${GREEN}Building $platform...${NC}"
    
    case $platform in
        desktop)
            if [ "$VERBOSE" = true ]; then
                cmake --build . --target desktop_dist
            else
                cmake --build . --target desktop_dist > /dev/null 2>&1
            fi
            echo -e "${GREEN}✓ Desktop build complete${NC}"
            ;;
        android)
            if [ "$VERBOSE" = true ]; then
                cmake --build . --target android_apk
                cmake --build . --target android_bundle
            else
                cmake --build . --target android_apk > /dev/null 2>&1
                cmake --build . --target android_bundle > /dev/null 2>&1
            fi
            echo -e "${GREEN}✓ Android build complete${NC}"
            ;;
        web)
            if [ "$VERBOSE" = true ]; then
                cmake --build . --target web_dist
            else
                cmake --build . --target web_dist > /dev/null 2>&1
            fi
            echo -e "${GREEN}✓ Web build complete${NC}"
            ;;
        ios)
            if [ "$VERBOSE" = true ]; then
                cmake --build . --target ios_ipa
            else
                cmake --build . --target ios_ipa > /dev/null 2>&1
            fi
            echo -e "${GREEN}✓ iOS build complete${NC}"
            ;;
    esac
done

# Package all
echo -e "${GREEN}Packaging builds...${NC}"
if [ "$VERBOSE" = true ]; then
    cmake --build . --target package_all
else
    cmake --build . --target package_all > /dev/null 2>&1
fi

# Show output location
echo ""
echo -e "${GREEN}Build complete!${NC}"
echo -e "Output files: ${YELLOW}$BUILD_DIR/dist/${NC}"
echo ""
ls -la dist/ 2>/dev/null || true

# Run if requested
if [ -n "$RUN_PLATFORM" ]; then
    echo -e "${GREEN}Running $RUN_PLATFORM...${NC}"
    case $RUN_PLATFORM in
        desktop)
            cmake --build . --target run_desktop
            ;;
        android)
            cmake --build . --target run_android
            ;;
        web)
            echo -e "${YELLOW}Starting web server at http://localhost:8000${NC}"
            cmake --build . --target serve_web
            ;;
        ios)
            cmake --build . --target run_ios_sim
            ;;
        *)
            echo -e "${RED}Unknown platform to run: $RUN_PLATFORM${NC}"
            ;;
    esac
fi