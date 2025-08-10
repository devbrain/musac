package com.musac;

/**
 * Audio format specification (matches musac::audio_format enum)
 */
public class AudioFormat {
    // Audio format values (matching musac::audio_format)
    public static final int UNKNOWN = 0x0000;
    public static final int U8 = 0x0008;      // Unsigned 8-bit
    public static final int S8 = 0x8008;      // Signed 8-bit  
    public static final int S16LE = 0x8010;   // Signed 16-bit little-endian
    public static final int S16BE = 0x9010;   // Signed 16-bit big-endian
    public static final int S32LE = 0x8020;   // Signed 32-bit little-endian
    public static final int S32BE = 0x9020;   // Signed 32-bit big-endian
    public static final int F32LE = 0x8120;   // Float 32-bit little-endian
    public static final int F32BE = 0x9120;   // Float 32-bit big-endian
    
    public int value;
    
    public AudioFormat() {
        this.value = S16LE;
    }
    
    public AudioFormat(int value) {
        this.value = value;
    }
    
    public int getBitSize() {
        return value & 0xFF;
    }
    
    public int getBytesPerSample() {
        return getBitSize() / 8;
    }
    
    public boolean isFloat() {
        return (value & 0x0100) != 0;
    }
    
    public boolean isSigned() {
        return (value & 0x8000) != 0;
    }
    
    public boolean isBigEndian() {
        return (value & 0x1000) != 0;
    }
    
    @Override
    public String toString() {
        switch (value) {
            case U8: return "U8";
            case S8: return "S8";
            case S16LE: return "S16LE";
            case S16BE: return "S16BE";
            case S32LE: return "S32LE";
            case S32BE: return "S32BE";
            case F32LE: return "F32LE";
            case F32BE: return "F32BE";
            case UNKNOWN: return "Unknown";
            default: return String.format("Format(0x%04X)", value);
        }
    }
}