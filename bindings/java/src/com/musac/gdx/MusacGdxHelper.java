package com.musac.gdx;

import com.musac.*;
import java.io.IOException;

/**
 * Helper class for integrating Musac with libGDX
 * 
 * This class provides convenience methods for using Musac decoders
 * with libGDX's audio system. It does not depend on libGDX directly,
 * so users need to handle the libGDX-specific parts themselves.
 */
public class MusacGdxHelper {
    
    /**
     * Decode an entire audio file suitable for libGDX Sound
     * Sound objects are for short audio clips that fit in memory
     * 
     * @param data Audio file data
     * @return DecodedAudio with PCM data and format info
     * @throws IOException if decoding fails
     */
    public static DecodedAudio decodeSound(byte[] data) throws IOException {
        try (MusacDecoder decoder = MusacDecoderFactory.create(data)) {
            AudioSpec spec = decoder.getSpec();
            float[] samples = decoder.decodeToFloat();
            
            return new DecodedAudio(samples, spec.freq, spec.channels);
        }
    }
    
    /**
     * Create a streaming decoder suitable for libGDX Music
     * Music objects are for longer audio that streams from disk
     * 
     * @param data Audio file data
     * @return StreamingDecoder that can be polled for audio chunks
     * @throws IOException if decoder creation fails
     */
    public static StreamingDecoder createStreamingDecoder(byte[] data) throws IOException {
        return new StreamingDecoder(data);
    }
    
    /**
     * Container for decoded audio data
     */
    public static class DecodedAudio {
        public final float[] samples;
        public final int sampleRate;
        public final int channels;
        
        public DecodedAudio(float[] samples, int sampleRate, int channels) {
            this.samples = samples;
            this.sampleRate = sampleRate;
            this.channels = channels;
        }
        
        public boolean isStereo() {
            return channels == 2;
        }
        
        public float getDuration() {
            return (float) samples.length / (sampleRate * channels);
        }
        
        /**
         * Convert to 16-bit PCM for compatibility
         */
        public short[] toShortArray() {
            return PcmUtils.floatToShort(samples);
        }
    }
    
    /**
     * Streaming decoder for large audio files
     */
    public static class StreamingDecoder implements AutoCloseable {
        private final MusacDecoder decoder;
        private final AudioSpec spec;
        private final int bufferSize;
        
        public StreamingDecoder(byte[] data) throws IOException {
            this(data, 4096);
        }
        
        public StreamingDecoder(byte[] data, int bufferSize) throws IOException {
            this.decoder = MusacDecoderFactory.create(data);
            this.spec = decoder.getSpec();
            this.bufferSize = bufferSize;
        }
        
        /**
         * Read next chunk of audio samples
         * @return Float samples or null if end of stream
         */
        public float[] readSamples() {
            float[] samples = decoder.decodeFloat(bufferSize);
            if (samples == null || samples.length == 0) {
                return null;
            }
            return samples;
        }
        
        /**
         * Read next chunk as 16-bit samples
         * @return Short samples or null if end of stream
         */
        public short[] readSamplesShort() {
            short[] samples = decoder.decodeShort(bufferSize);
            if (samples == null || samples.length == 0) {
                return null;
            }
            return samples;
        }
        
        /**
         * Seek to position in seconds
         */
        public void seek(float seconds) {
            decoder.seek(seconds);
        }
        
        /**
         * Reset to beginning
         */
        public void reset() {
            decoder.rewind();
        }
        
        /**
         * Get total duration in seconds
         */
        public float getDuration() {
            return (float) decoder.getDuration();
        }
        
        /**
         * Get sample rate
         */
        public int getSampleRate() {
            return spec.freq;
        }
        
        /**
         * Get number of channels
         */
        public int getChannels() {
            return spec.channels;
        }
        
        /**
         * Check if stereo
         */
        public boolean isStereo() {
            return spec.channels == 2;
        }
        
        @Override
        public void close() {
            decoder.close();
        }
    }
    
    /**
     * Example usage with libGDX (user needs to implement):
     * 
     * // For Sound (short clips):
     * DecodedAudio audio = MusacGdxHelper.decodeSound(fileBytes);
     * AudioDevice device = Gdx.audio.newAudioDevice(
     *     audio.sampleRate, 
     *     audio.isStereo()
     * );
     * device.writeSamples(audio.samples, 0, audio.samples.length);
     * 
     * // For Music (streaming):
     * StreamingDecoder decoder = MusacGdxHelper.createStreamingDecoder(fileBytes);
     * AudioDevice device = Gdx.audio.newAudioDevice(
     *     decoder.getSampleRate(),
     *     decoder.isStereo()
     * );
     * 
     * // In render loop:
     * float[] chunk = decoder.readSamples();
     * if (chunk != null) {
     *     device.writeSamples(chunk, 0, chunk.length);
     * }
     */
}