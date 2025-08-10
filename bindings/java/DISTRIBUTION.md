# Musac Distribution Strategy for libGDX

## Distribution Options

### Option 1: Maven Central / JitPack (Recommended)

The most convenient way for libGDX users is through Maven/Gradle dependency management.

#### A. JitPack (Easier to setup)

**Setup `build.gradle` for multi-platform distribution:**

```gradle
// root build.gradle
allprojects {
    repositories {
        maven { url 'https://jitpack.io' }
    }
}

// core module
dependencies {
    api "com.github.yourusername.musac:musac-core:1.0.0"
}

// desktop module  
dependencies {
    implementation "com.github.yourusername.musac:musac-desktop:1.0.0"
    runtimeOnly "com.github.yourusername.musac:musac-natives-desktop:1.0.0"
}

// android module
dependencies {
    implementation "com.github.yourusername.musac:musac-android:1.0.0"
}

// html module
dependencies {
    implementation "com.github.yourusername.musac:musac-gwt:1.0.0"
    implementation "com.github.yourusername.musac:musac-gwt:1.0.0:sources"
}
```

#### B. Maven Central (Professional)

Requires more setup but is the standard for production libraries.

### Option 2: GitHub Releases with Gradle Plugin

Create a custom Gradle plugin that automatically downloads and configures the appropriate binaries.

```gradle
plugins {
    id 'com.musac.libgdx' version '1.0.0'
}

musac {
    version = "1.0.0"
    platforms = ['desktop', 'android', 'ios', 'html']
}
```

### Option 3: Single Fat JAR Distribution

Bundle everything in one JAR with platform detection.

---

## Recommended Structure: Modular Maven Artifacts

### 1. Core Module (Platform-independent)
**`musac-core-1.0.0.jar`**
```
com/musac/
├── AudioFormat.java
├── AudioSpec.java
├── MusacDecoder.java
├── MusacDecoderFactory.java
├── PcmUtils.java
├── decoders/*.java
└── gdx/MusacGdxHelper.java
```

### 2. Desktop Module
**`musac-desktop-1.0.0.jar`**
```
com/musac/desktop/
└── DesktopMusacLoader.java
```

**`musac-natives-desktop-1.0.0.jar`**
```
natives/
├── linux-x86-64/libmusac_java.so
├── windows-x86-64/musac_java.dll
├── windows-x86/musac_java.dll
└── macos/libmusac_java.dylib
```

### 3. Android Module
**`musac-android-1.0.0.aar`**
```
jni/
├── armeabi-v7a/libmusac_java.so
├── arm64-v8a/libmusac_java.so
├── x86/libmusac_java.so
└── x86_64/libmusac_java.so
classes.jar
AndroidManifest.xml
```

### 4. iOS Module
**`musac-ios-1.0.0.jar`**
```
com/musac/ios/
└── IOSMusacLoader.java
libmusac_java.a (static library)
```

### 5. GWT/HTML Module
**`musac-gwt-1.0.0.jar`**
**`musac-gwt-1.0.0-sources.jar`**
```
com/musac/gwt/
├── MusacGWT.java
└── Musac.gwt.xml
resources/
├── musac.js
├── musac_wasm.js
└── musac_wasm.wasm
```

---

## Implementation: Gradle Build Files

### `/bindings/java/build.gradle`

```gradle
plugins {
    id 'java-library'
    id 'maven-publish'
}

group = 'com.github.yourusername'
version = '1.0.0'

subprojects {
    apply plugin: 'java-library'
    apply plugin: 'maven-publish'
    
    java {
        sourceCompatibility = JavaVersion.VERSION_1_8
        targetCompatibility = JavaVersion.VERSION_1_8
        withSourcesJar()
        withJavadocJar()
    }
}

// Platform detection
ext {
    osName = System.getProperty('os.name').toLowerCase()
    isWindows = osName.contains('windows')
    isLinux = osName.contains('linux')
    isMac = osName.contains('mac')
    is64Bit = System.getProperty('os.arch').contains('64')
}
```

### `/bindings/java/core/build.gradle`

```gradle
project(':core') {
    dependencies {
        // No dependencies - pure Java
    }
    
    sourceSets {
        main {
            java {
                srcDirs = ['../src']
                exclude '**/android/**'
                exclude '**/gwt/**'
            }
        }
    }
    
    publishing {
        publications {
            maven(MavenPublication) {
                artifactId = 'musac-core'
                from components.java
            }
        }
    }
}
```

### `/bindings/java/desktop/build.gradle`

```gradle
project(':desktop') {
    dependencies {
        api project(':core')
    }
    
    task packNatives(type: Jar) {
        classifier = 'natives-desktop'
        from('../build/bin') {
            include '**/*.so'
            include '**/*.dll'
            include '**/*.dylib'
            into 'natives'
        }
    }
    
    artifacts {
        archives packNatives
    }
    
    publishing {
        publications {
            maven(MavenPublication) {
                artifactId = 'musac-desktop'
                from components.java
                
                artifact packNatives {
                    classifier 'natives-desktop'
                }
            }
        }
    }
}
```

### `/bindings/java/android/build.gradle`

```gradle
apply plugin: 'com.android.library'

android {
    // ... android config
}

afterEvaluate {
    publishing {
        publications {
            release(MavenPublication) {
                from components.release
                artifactId = 'musac-android'
                
                pom {
                    packaging = 'aar'
                }
            }
        }
    }
}
```

### `/bindings/java/gwt/build.gradle`

```gradle
project(':gwt') {
    dependencies {
        api project(':core')
    }
    
    sourceSets {
        main {
            java {
                srcDirs = ['../web/src']
            }
            resources {
                srcDirs = ['../web/src', '../web/build/dist']
                include '**/*.gwt.xml'
                include '**/*.js'
                include '**/*.wasm'
            }
        }
    }
    
    publishing {
        publications {
            maven(MavenPublication) {
                artifactId = 'musac-gwt'
                from components.java
            }
        }
    }
}
```

---

## libGDX Integration Guide

### Step 1: Add Dependencies

**`build.gradle` (root)**
```gradle
allprojects {
    ext {
        musacVersion = '1.0.0'
    }
    
    repositories {
        mavenCentral()
        maven { url 'https://jitpack.io' }
    }
}
```

**`core/build.gradle`**
```gradle
dependencies {
    api "com.github.yourusername.musac:musac-core:$musacVersion"
}
```

**`desktop/build.gradle`**
```gradle
dependencies {
    implementation project(':core')
    implementation "com.github.yourusername.musac:musac-desktop:$musacVersion"
    runtimeOnly "com.github.yourusername.musac:musac-desktop:$musacVersion:natives-desktop"
}

// Extract natives
task extractNatives(type: Copy) {
    from configurations.runtimeClasspath
    include "**/*natives-desktop*.jar"
    into "$buildDir/natives"
    
    doLast {
        file("$buildDir/natives").eachFile { jar ->
            copy {
                from zipTree(jar)
                into "$buildDir/natives"
            }
        }
    }
}

run.dependsOn extractNatives
run.doFirst {
    jvmArgs += "-Djava.library.path=$buildDir/natives"
}
```

**`android/build.gradle`**
```gradle
dependencies {
    implementation project(':core')
    implementation "com.github.yourusername.musac:musac-android:$musacVersion"
}
```

**`html/build.gradle`**
```gradle
dependencies {
    implementation project(':core')
    implementation "com.github.yourusername.musac:musac-gwt:$musacVersion"
    implementation "com.github.yourusername.musac:musac-gwt:$musacVersion:sources"
}
```

**`html/src/GdxDefinition.gwt.xml`**
```xml
<module>
    <!-- ... existing content ... -->
    <inherits name='com.musac.Musac' />
</module>
```

### Step 2: Platform-Specific Initialization

**Desktop Launcher:**
```java
public class DesktopLauncher {
    public static void main(String[] args) {
        // Musac auto-initializes on desktop
        Lwjgl3ApplicationConfiguration config = new Lwjgl3ApplicationConfiguration();
        new Lwjgl3Application(new MyGdxGame(), config);
    }
}
```

**Android Launcher:**
```java
public class AndroidLauncher extends AndroidApplication {
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        
        // Initialize Musac
        MusacAndroidHelper.init(this);
        
        AndroidApplicationConfiguration config = new AndroidApplicationConfiguration();
        initialize(new MyGdxGame(), config);
    }
}
```

**HTML Launcher:**
```java
public class HtmlLauncher extends GwtApplication {
    @Override
    public void onModuleLoad() {
        // Initialize Musac WASM
        MusacGWT.initialize();
        super.onModuleLoad();
    }
    
    @Override
    public GwtApplicationConfiguration getConfig() {
        return new GwtApplicationConfiguration(800, 600);
    }
    
    @Override
    public ApplicationListener createApplicationListener() {
        return new MyGdxGame();
    }
}
```

### Step 3: Use in Game Code

**Core Module:**
```java
public class MyGdxGame extends ApplicationAdapter {
    private MusacDecoder decoder;
    private AudioDevice audioDevice;
    
    @Override
    public void create() {
        // Load audio file
        FileHandle file = Gdx.files.internal("music.ogg");
        byte[] data = file.readBytes();
        
        // Decode with Musac
        try {
            decoder = MusacDecoderFactory.create(data);
            AudioSpec spec = decoder.getSpec();
            
            // Create libGDX audio device
            audioDevice = Gdx.audio.newAudioDevice(
                spec.freq,
                spec.channels == 2
            );
            
            // Play audio
            float[] samples = decoder.decodeFloat(4096);
            audioDevice.writeSamples(samples, 0, samples.length);
            
        } catch (IOException e) {
            Gdx.app.error("Audio", "Failed to decode", e);
        }
    }
    
    @Override
    public void dispose() {
        if (decoder != null) decoder.close();
        if (audioDevice != null) audioDevice.dispose();
    }
}
```

---

## Publishing to JitPack

### 1. Create GitHub Release
```bash
git tag -a v1.0.0 -m "Release version 1.0.0"
git push origin v1.0.0
```

### 2. Create `jitpack.yml`
```yaml
jdk:
  - openjdk11
  
before_install:
  # Install Android SDK if needed
  - if [ "$PLATFORM" = "android" ]; then
      wget https://dl.google.com/android/repository/commandlinetools-linux-latest.zip;
      unzip commandlinetools-linux-latest.zip;
      yes | sdkmanager --licenses;
    fi
  
  # Install Emscripten if needed for GWT
  - if [ "$PLATFORM" = "gwt" ]; then
      git clone https://github.com/emscripten-core/emsdk.git;
      cd emsdk && ./emsdk install latest && ./emsdk activate latest;
      source ./emsdk_env.sh;
      cd ..;
    fi

install:
  - ./gradlew clean publishToMavenLocal
```

### 3. Test with JitPack
Visit: `https://jitpack.io/#yourusername/musac`

---

## Single File Distribution (Alternative)

For simplicity, we could also provide a single "fat JAR" that includes everything:

**`musac-all-1.0.0.jar`**
- All Java classes
- All native libraries
- Platform detection code
- Auto-extraction on first use

```java
public class MusacLoader {
    static {
        // Detect platform and extract appropriate native library
        String os = System.getProperty("os.name").toLowerCase();
        String arch = System.getProperty("os.arch");
        String libName = getLibraryName(os, arch);
        
        // Extract from JAR to temp directory
        extractAndLoad(libName);
    }
}
```

---

## Recommended Approach

1. **Use JitPack** for easy distribution
2. **Modular artifacts** for optimal size
3. **Platform-specific natives** only where needed
4. **Clear documentation** with examples
5. **Version compatibility** matrix with libGDX versions

This provides the best balance of:
- Easy integration
- Small download size
- Platform flexibility
- Professional distribution