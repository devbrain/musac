#!/bin/bash

# Create a mock AAR structure for demonstration
# In production, this would be created by the actual Android build

set -e

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
AAR_DIR="$SCRIPT_DIR/mock_aar"
AAR_NAME="musac-android-1.0.aar"

echo "Creating mock AAR structure for demonstration..."

# Clean and create directories
rm -rf "$AAR_DIR"
mkdir -p "$AAR_DIR"
cd "$AAR_DIR"

# Create AAR structure
mkdir -p {classes,jni,res,libs,assets}

# Copy Java classes (would be compiled .class files)
echo "Copying Java classes..."
mkdir -p classes/com/musac
cp -r "$SCRIPT_DIR/../src/com/musac" classes/com/ 2>/dev/null || true

# Create mock native libraries structure
echo "Creating native library structure..."
for ABI in armeabi-v7a arm64-v8a x86 x86_64; do
    mkdir -p "jni/$ABI"
    # Create placeholder (in real build, actual .so files would be here)
    echo "Native library for $ABI" > "jni/$ABI/libmusac_java.so.placeholder"
done

# Create AndroidManifest.xml
cat > AndroidManifest.xml << 'EOF'
<?xml version="1.0" encoding="utf-8"?>
<manifest xmlns:android="http://schemas.android.com/apk/res/android"
    package="com.musac"
    android:versionCode="1"
    android:versionName="1.0">
    
    <uses-sdk
        android:minSdkVersion="21"
        android:targetSdkVersion="34" />
        
</manifest>
EOF

# Create classes.jar (would contain compiled Java classes)
echo "Creating classes.jar..."
cd classes
jar cf ../classes.jar com/
cd ..

# Create R.txt (resources index)
touch R.txt

# Create proguard.txt
cat > proguard.txt << 'EOF'
# Musac library ProGuard rules
-keep class com.musac.** { *; }
-keepclassmembers class com.musac.** {
    native <methods>;
}
EOF

# Package as AAR
echo "Creating AAR package..."
zip -r "../$AAR_NAME" . -x "classes/*"

cd ..
echo "Mock AAR created: $AAR_DIR/$AAR_NAME"

# Show structure
echo ""
echo "AAR Structure:"
unzip -l "$AAR_NAME" | head -20

echo ""
echo "Note: This is a mock AAR for demonstration."
echo "To build the actual AAR with native libraries, you need:"
echo "1. Android NDK installed"
echo "2. Run ./build_android.sh or use Android Studio"