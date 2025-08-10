package com.musac;

/**
 * Utilities for PCM audio data conversion
 */
public class PcmUtils {
    
    /**
     * Convert float samples [-1.0, 1.0] to 16-bit signed integers
     * @param floats Float samples
     * @return 16-bit samples
     */
    public static short[] floatToShort(float[] floats) {
        if (floats == null) return null;
        
        short[] shorts = new short[floats.length];
        for (int i = 0; i < floats.length; i++) {
            float sample = floats[i];
            // Clamp to [-1, 1]
            if (sample > 1.0f) sample = 1.0f;
            if (sample < -1.0f) sample = -1.0f;
            shorts[i] = (short)(sample * 32767.0f);
        }
        return shorts;
    }
    
    /**
     * Convert 16-bit signed integers to float samples [-1.0, 1.0]
     * @param shorts 16-bit samples
     * @return Float samples
     */
    public static float[] shortToFloat(short[] shorts) {
        if (shorts == null) return null;
        
        float[] floats = new float[shorts.length];
        for (int i = 0; i < shorts.length; i++) {
            floats[i] = shorts[i] / 32768.0f;
        }
        return floats;
    }
    
    /**
     * Convert byte array (little-endian 16-bit) to short array
     * @param bytes Byte array
     * @return Short array
     */
    public static short[] bytesToShort(byte[] bytes) {
        if (bytes == null || bytes.length % 2 != 0) return null;
        
        short[] shorts = new short[bytes.length / 2];
        for (int i = 0; i < shorts.length; i++) {
            int idx = i * 2;
            shorts[i] = (short)((bytes[idx] & 0xFF) | ((bytes[idx + 1] & 0xFF) << 8));
        }
        return shorts;
    }
    
    /**
     * Convert short array to byte array (little-endian 16-bit)
     * @param shorts Short array
     * @return Byte array
     */
    public static byte[] shortToBytes(short[] shorts) {
        if (shorts == null) return null;
        
        byte[] bytes = new byte[shorts.length * 2];
        for (int i = 0; i < shorts.length; i++) {
            int idx = i * 2;
            bytes[idx] = (byte)(shorts[i] & 0xFF);
            bytes[idx + 1] = (byte)((shorts[i] >> 8) & 0xFF);
        }
        return bytes;
    }
    
    /**
     * Interleave stereo channels
     * @param left Left channel samples
     * @param right Right channel samples
     * @return Interleaved stereo samples
     */
    public static float[] interleaveStereo(float[] left, float[] right) {
        if (left == null || right == null) return null;
        int len = Math.min(left.length, right.length);
        
        float[] interleaved = new float[len * 2];
        for (int i = 0; i < len; i++) {
            interleaved[i * 2] = left[i];
            interleaved[i * 2 + 1] = right[i];
        }
        return interleaved;
    }
    
    /**
     * Deinterleave stereo channels
     * @param interleaved Interleaved stereo samples
     * @return Array with [0]=left channel, [1]=right channel
     */
    public static float[][] deinterleaveStereo(float[] interleaved) {
        if (interleaved == null || interleaved.length % 2 != 0) return null;
        
        int len = interleaved.length / 2;
        float[] left = new float[len];
        float[] right = new float[len];
        
        for (int i = 0; i < len; i++) {
            left[i] = interleaved[i * 2];
            right[i] = interleaved[i * 2 + 1];
        }
        
        return new float[][] { left, right };
    }
    
    /**
     * Convert stereo to mono by averaging channels
     * @param stereo Interleaved stereo samples
     * @return Mono samples
     */
    public static float[] stereoToMono(float[] stereo) {
        if (stereo == null || stereo.length % 2 != 0) return null;
        
        float[] mono = new float[stereo.length / 2];
        for (int i = 0; i < mono.length; i++) {
            mono[i] = (stereo[i * 2] + stereo[i * 2 + 1]) * 0.5f;
        }
        return mono;
    }
    
    /**
     * Convert mono to stereo by duplicating channel
     * @param mono Mono samples
     * @return Interleaved stereo samples
     */
    public static float[] monoToStereo(float[] mono) {
        if (mono == null) return null;
        
        float[] stereo = new float[mono.length * 2];
        for (int i = 0; i < mono.length; i++) {
            stereo[i * 2] = mono[i];
            stereo[i * 2 + 1] = mono[i];
        }
        return stereo;
    }
    
    /**
     * Normalize audio samples to maximum amplitude
     * @param samples Audio samples
     * @param targetPeak Target peak level (0.0 to 1.0)
     */
    public static void normalize(float[] samples, float targetPeak) {
        if (samples == null || samples.length == 0) return;
        
        // Find peak
        float peak = 0;
        for (float sample : samples) {
            float abs = Math.abs(sample);
            if (abs > peak) peak = abs;
        }
        
        if (peak == 0) return;
        
        // Scale to target peak
        float scale = targetPeak / peak;
        for (int i = 0; i < samples.length; i++) {
            samples[i] *= scale;
        }
    }
}