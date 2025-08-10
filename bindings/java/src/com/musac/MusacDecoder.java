package com.musac;

import java.io.Closeable;
import java.io.IOException;

/**
 * Main decoder class for decoding audio files
 */
public class MusacDecoder implements Closeable {
    private long nativeHandle = 0;
    
    /**
     * Create a decoder from audio data (auto-detect format)
     * @param data Audio file data
     * @throws IOException if format cannot be detected or decoder creation fails
     */
    public MusacDecoder(byte[] data) throws IOException {
        if (!MusacNative.isInitialized()) {
            MusacNative.init();
        }
        
        nativeHandle = nativeCreateFromData(data);
        if (nativeHandle == 0) {
            throw new IOException("Failed to create decoder from data");
        }
    }
    
    /**
     * Protected constructor for subclasses
     */
    protected MusacDecoder() {
    }
    
    /**
     * Get the audio specification
     * @return Audio spec with format, sample rate, channels
     */
    public AudioSpec getSpec() {
        checkHandle();
        return nativeGetSpec(nativeHandle);
    }
    
    /**
     * Decode audio samples as 16-bit integers
     * @param samples Number of samples to decode
     * @return Array of decoded samples (interleaved if stereo)
     */
    public short[] decodeShort(int samples) {
        checkHandle();
        return nativeDecodeShort(nativeHandle, samples);
    }
    
    /**
     * Decode audio samples as floating point [-1.0, 1.0]
     * @param samples Number of samples to decode
     * @return Array of decoded samples (interleaved if stereo)
     */
    public float[] decodeFloat(int samples) {
        checkHandle();
        return nativeDecodeFloat(nativeHandle, samples);
    }
    
    /**
     * Decode entire audio file to short array
     * @return All audio samples as 16-bit integers
     */
    public short[] decodeToShort() {
        checkHandle();
        // Decode in chunks and combine
        java.util.List<short[]> chunks = new java.util.ArrayList<>();
        int totalSamples = 0;
        
        while (true) {
            short[] chunk = decodeShort(4096);
            if (chunk == null || chunk.length == 0) {
                break;
            }
            chunks.add(chunk);
            totalSamples += chunk.length;
        }
        
        // Combine chunks
        short[] result = new short[totalSamples];
        int pos = 0;
        for (short[] chunk : chunks) {
            System.arraycopy(chunk, 0, result, pos, chunk.length);
            pos += chunk.length;
        }
        
        return result;
    }
    
    /**
     * Decode entire audio file to float array
     * @return All audio samples as floats [-1.0, 1.0]
     */
    public float[] decodeToFloat() {
        checkHandle();
        // Decode in chunks and combine
        java.util.List<float[]> chunks = new java.util.ArrayList<>();
        int totalSamples = 0;
        
        while (true) {
            float[] chunk = decodeFloat(4096);
            if (chunk == null || chunk.length == 0) {
                break;
            }
            chunks.add(chunk);
            totalSamples += chunk.length;
        }
        
        // Combine chunks
        float[] result = new float[totalSamples];
        int pos = 0;
        for (float[] chunk : chunks) {
            System.arraycopy(chunk, 0, result, pos, chunk.length);
            pos += chunk.length;
        }
        
        return result;
    }
    
    /**
     * Seek to position in seconds
     * @param seconds Position to seek to
     */
    public void seek(double seconds) {
        checkHandle();
        nativeSeek(nativeHandle, seconds);
    }
    
    /**
     * Rewind to beginning
     */
    public void rewind() {
        seek(0.0);
    }
    
    /**
     * Get duration in seconds
     * @return Duration or -1 if unknown
     */
    public double getDuration() {
        checkHandle();
        return nativeGetDuration(nativeHandle);
    }
    
    /**
     * Get decoder name
     * @return Name of the decoder (e.g., "mp3", "vorbis")
     */
    public String getName() {
        checkHandle();
        return nativeGetName(nativeHandle);
    }
    
    /**
     * Close the decoder and release resources
     */
    @Override
    public void close() {
        if (nativeHandle != 0) {
            nativeDestroy(nativeHandle);
            nativeHandle = 0;
        }
    }
    
    @Override
    protected void finalize() throws Throwable {
        close();
        super.finalize();
    }
    
    private void checkHandle() {
        if (nativeHandle == 0) {
            throw new IllegalStateException("Decoder has been closed");
        }
    }
    
    // Native methods
    private static native long nativeCreate(String type);
    private static native long nativeCreateFromData(byte[] data);
    private native void nativeOpen(long handle, byte[] data);
    private native AudioSpec nativeGetSpec(long handle);
    private native short[] nativeDecodeShort(long handle, int samples);
    private native float[] nativeDecodeFloat(long handle, int samples);
    private native void nativeSeek(long handle, double seconds);
    private native double nativeGetDuration(long handle);
    private native String nativeGetName(long handle);
    private native void nativeDestroy(long handle);
}