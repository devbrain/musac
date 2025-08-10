package com.musac;

/**
 * Native library loader and utility methods
 */
public class MusacNative {
    private static boolean initialized = false;
    
    static {
        try {
            System.loadLibrary("musac_java");
            nativeInit();
            initialized = true;
        } catch (UnsatisfiedLinkError e) {
            System.err.println("Failed to load musac_java native library: " + e.getMessage());
            System.err.println("Make sure the library is in java.library.path");
        }
    }
    
    /**
     * Initialize the native library (called automatically)
     */
    private static native void nativeInit();
    
    /**
     * Detect audio format from data
     * @param data Audio file data
     * @return Format name (e.g., "mp3", "ogg", "wav") or null if unknown
     */
    public static native String nativeDetectFormat(byte[] data);
    
    /**
     * Check if extension can be decoded
     * @param extension File extension without dot (e.g., "mp3", "ogg")
     * @return true if the extension is supported
     */
    public static native boolean nativeCanDecodeExtension(String extension);
    
    /**
     * Check if the library is initialized
     */
    public static boolean isInitialized() {
        return initialized;
    }
    
    /**
     * Manually initialize the library if not already done
     */
    public static void init() {
        if (!initialized) {
            System.loadLibrary("musac_java");
            nativeInit();
            initialized = true;
        }
    }
    
    /**
     * Detect audio format from data
     * @param data Audio file data  
     * @return Format name or null
     */
    public static String detectFormat(byte[] data) {
        if (!initialized) {
            throw new IllegalStateException("MusacNative not initialized");
        }
        return nativeDetectFormat(data);
    }
    
    /**
     * Check if file extension can be decoded
     * @param extension File extension without dot
     * @return true if supported
     */
    public static boolean canDecode(String extension) {
        if (!initialized) {
            throw new IllegalStateException("MusacNative not initialized");
        }
        return nativeCanDecodeExtension(extension);
    }
}