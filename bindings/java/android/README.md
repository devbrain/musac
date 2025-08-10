# Musac Android Build

This directory contains the Android build configuration for the Musac JNI library.

## Prerequisites

1. Android NDK r26b or later
2. CMake 3.22 or later
3. Java 11 or later

## Building

### Option 1: Using Android Studio

1. Open Android Studio
2. Open this directory as a project
3. Android Studio will automatically download the NDK and build the library
4. The AAR file will be in `build/outputs/aar/`

### Option 2: Command Line with Android SDK

1. Set up your environment:
```bash
export ANDROID_SDK_ROOT=$HOME/Android/Sdk
export ANDROID_NDK_HOME=$ANDROID_SDK_ROOT/ndk/26.1.10909125
```

2. Run the build script:
```bash
./build_android.sh
```

### Option 3: Standalone NDK Build

1. Download Android NDK r26b:
```bash
wget https://dl.google.com/android/repository/android-ndk-r26b-linux.zip
unzip android-ndk-r26b-linux.zip
```

2. Run the standalone build:
```bash
./standalone_build.sh
```

## Output

The build produces:
- Native libraries (.so files) for each ABI in `src/main/jniLibs/`
- AAR package in `build/outputs/aar/` (when using Gradle)

## Supported ABIs

- armeabi-v7a (32-bit ARM)
- arm64-v8a (64-bit ARM)
- x86 (32-bit Intel, mainly for emulator)
- x86_64 (64-bit Intel, for emulator)

## Integration with libGDX

Add the AAR to your libGDX Android project:

```gradle
dependencies {
    implementation files('libs/musac-android.aar')
}
```

Then use the same Java API as on desktop:

```java
import com.musac.*;

// In your Android activity or libGDX game
byte[] audioData = loadAudioFile();
MusacDecoder decoder = MusacDecoderFactory.create(audioData);
AudioSpec spec = decoder.getSpec();
float[] samples = decoder.decodeToFloat();
```

## File Size

Approximate library sizes per ABI:
- armeabi-v7a: ~500 KB
- arm64-v8a: ~600 KB
- x86: ~550 KB
- x86_64: ~650 KB

Total AAR size with all ABIs: ~2.5 MB

## Optimization

For smaller APK size, you can use ABI filters in your app's build.gradle:

```gradle
android {
    defaultConfig {
        ndk {
            // Only include ABIs you need
            abiFilters 'arm64-v8a', 'armeabi-v7a'
        }
    }
}
```

Or use App Bundle to let Google Play deliver only the needed ABI.