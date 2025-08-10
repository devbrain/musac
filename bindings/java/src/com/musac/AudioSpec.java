package com.musac;

/**
 * Audio specification including format, sample rate, and channels
 */
public class AudioSpec {
    public int freq;        // Sample rate in Hz
    public int channels;    // Number of channels (1=mono, 2=stereo)
    public int samples;     // Buffer size in samples
    public AudioFormat format;
    
    public AudioSpec() {
        this.freq = 44100;
        this.channels = 2;
        this.samples = 4096;
        this.format = new AudioFormat();
    }
    
    public AudioSpec(int freq, int channels, AudioFormat format) {
        this.freq = freq;
        this.channels = channels;
        this.samples = 4096; // Default buffer size
        this.format = format;
    }
    
    public boolean isStereo() {
        return channels == 2;
    }
    
    public boolean isMono() {
        return channels == 1;
    }
    
    public int getBytesPerFrame() {
        return channels * format.getBytesPerSample();
    }
    
    public double samplesToSeconds(int sampleCount) {
        return (double) sampleCount / (freq * channels);
    }
    
    public int secondsToSamples(double seconds) {
        return (int) (seconds * freq * channels);
    }
    
    @Override
    public String toString() {
        return String.format("AudioSpec(%d Hz, %d ch, %d samples, %s)",
                           freq, channels, samples, format);
    }
}