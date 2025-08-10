# Building Musac libGDX Demo for All Platforms

This project can be built for all libGDX platforms using CMake. The build system automatically handles native library compilation, Java/Gradle builds, and packaging.

## Prerequisites

### Common Requirements
- CMake 3.16 or higher
- Java 8 or higher
- Git

### Platform-Specific Requirements

#### Desktop (Windows/Linux/macOS)
- C++ compiler (GCC, Clang, or MSVC)
- No additional requirements

#### Android
- Android SDK (set `ANDROID_SDK_ROOT` or `ANDROID_HOME`)
- Android NDK (set `ANDROID_NDK_ROOT` or use SDK's NDK)
- Android Studio (optional, for development)

#### Web/HTML5
- Emscripten SDK (install and source emsdk)
  ```bash
  git clone https://github.com/emscripten-core/emsdk.git
  cd emsdk
  ./emsdk install latest
  ./emsdk activate latest
  source ./emsdk_env.sh
  ```

#### iOS (macOS only)
- Xcode with iOS SDK
- RoboVM (automatically downloaded by Gradle)

## Quick Build

### Using the build script (Recommended)

#### Linux/macOS:
```bash
# Build all platforms
./build.sh all

# Build specific platforms
./build.sh desktop android

# Clean, build and run desktop
./build.sh -c -r desktop desktop

# Build debug version for Android
./build.sh -d android

# Verbose build
./build.sh -v desktop
```

#### Windows:
```cmd
# Build all platforms
build.bat all

# Build specific platforms
build.bat desktop android

# Clean, build and run desktop
build.bat -c -r desktop desktop

# Build debug version for Android
build.bat -d android
```

## Manual CMake Build

### 1. Configure

```bash
mkdir build
cd build

# Configure for all platforms
cmake .. -DBUILD_DESKTOP=ON -DBUILD_ANDROID=ON -DBUILD_WEB=ON -DBUILD_IOS=ON

# Or configure for specific platforms
cmake .. -DBUILD_DESKTOP=ON -DBUILD_ANDROID=ON
```

### 2. Build

```bash
# Build all configured platforms
cmake --build . --target build_all

# Or build specific targets
cmake --build . --target desktop_dist
cmake --build . --target android_apk
cmake --build . --target web_dist
cmake --build . --target ios_ipa
```

### 3. Run

```bash
# Desktop
cmake --build . --target run_desktop

# Android (device must be connected)
cmake --build . --target run_android

# Web (starts local server)
cmake --build . --target serve_web

# iOS Simulator
cmake --build . --target run_ios_sim
```

## Build Targets

### Desktop Targets
- `desktop_jar` - Build runnable JAR
- `desktop_dist` - Build distribution package
- `desktop_exe` - Windows executable (Windows only)
- `desktop_app` - macOS app bundle (macOS only)
- `desktop_linux` - Linux package (Linux only)
- `run_desktop` - Run desktop version

### Android Targets
- `android_apk` - Build debug APK
- `android_bundle` - Build release AAB
- `install_android` - Install to connected device
- `run_android` - Install and run on device

### Web Targets
- `web_compile` - Compile GWT
- `web_dist` - Build production web version
- `run_web_dev` - Start GWT development server
- `serve_web` - Serve production build

### iOS Targets
- `ios_build` - Build iOS app
- `ios_ipa` - Create IPA package
- `deploy_ios` - Deploy to connected device
- `run_ios_sim` - Run in iOS simulator

### Utility Targets
- `build_all` - Build all configured platforms
- `package_all` - Package all builds to dist/
- `clean_all` - Clean all build artifacts
- `help_platforms` - Show all available targets

## Output Structure

After building, all outputs are organized in the `dist/` directory:

```
build/dist/
├── desktop/
│   ├── musac_demo-1.0.0.jar
│   └── [platform-specific packages]
├── android/
│   ├── musac_demo-debug.apk
│   └── musac_demo-release.aab
├── web/
│   ├── index.html
│   ├── musac.js
│   ├── musac.wasm
│   └── [GWT compiled files]
└── ios/
    └── musac_demo.ipa
```

## Platform-Specific Notes

### Desktop
- The JAR file includes all dependencies and native libraries
- Double-click to run on systems with Java installed
- Native packages (.exe, .app, .deb) are created for each platform

### Android
- APK is for testing (debug build)
- AAB is for Google Play Store (release build)
- Minimum API level: 21 (Android 5.0)
- Supports all ABIs: armeabi-v7a, arm64-v8a, x86, x86_64

### Web
- Uses WebAssembly for Musac codec support
- Falls back to Web Audio API for standard formats
- Requires modern browser with WASM support
- GWT compilation may take several minutes

### iOS
- Requires macOS for building
- IPA can be deployed via TestFlight or Ad Hoc
- Supports iPhone and iPad
- Minimum iOS version: 11.0

## Troubleshooting

### CMake not finding Java
```bash
export JAVA_HOME=/path/to/java
cmake .. -DJAVA_HOME=$JAVA_HOME
```

### Android build fails
```bash
# Ensure SDK and NDK are set
export ANDROID_SDK_ROOT=$HOME/Android/Sdk
export ANDROID_NDK_ROOT=$ANDROID_SDK_ROOT/ndk/26.1.10909125
```

### Web build fails
```bash
# Ensure Emscripten is activated
source /path/to/emsdk/emsdk_env.sh
```

### Gradle daemon issues
```bash
./gradlew --stop
./gradlew clean
```

## Development Tips

1. **Incremental builds**: After initial build, subsequent builds are faster
2. **Platform testing**: Use emulators/simulators for quick testing
3. **Debug builds**: Use `-DCMAKE_BUILD_TYPE=Debug` for debugging
4. **Parallel builds**: Add `-j8` to build commands for faster compilation

## CI/CD Integration

The CMake build system can be easily integrated with CI/CD pipelines:

```yaml
# GitHub Actions example
- name: Build all platforms
  run: |
    mkdir build
    cd build
    cmake .. -DBUILD_DESKTOP=ON -DBUILD_ANDROID=ON -DBUILD_WEB=ON
    cmake --build . --target build_all
    cmake --build . --target package_all
```

## Support

For issues or questions:
1. Check the build logs in `build/` directory
2. Run with verbose flag: `./build.sh -v [platform]`
3. Ensure all prerequisites are installed and environment variables are set