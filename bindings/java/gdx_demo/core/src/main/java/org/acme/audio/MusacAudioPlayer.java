package org.acme.audio;

import com.badlogic.gdx.audio.AudioDevice;
import com.badlogic.gdx.Gdx;
import com.badlogic.gdx.files.FileHandle;
import com.musac.MusacDecoder;
import com.musac.AudioFormat;
import com.musac.AudioSpec;

import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicReference;

/**
 * High-quality audio player using Musac native decoders
 * Supports advanced formats like MOD, MIDI, VGM
 * Available on Desktop and Android platforms
 */
public class MusacAudioPlayer implements AudioPlayer {
    private static final int BUFFER_SIZE = 4096;
    
    private MusacDecoder decoder;
    private AudioDevice audioDevice;
    private Thread playbackThread;
    private final AtomicBoolean playing = new AtomicBoolean(false);
    private final AtomicBoolean paused = new AtomicBoolean(false);
    private final AtomicReference<PlaybackListener> listener = new AtomicReference<>();
    
    private AudioSpec audioSpec;
    private float volume = 1.0f;
    private long totalSamples;
    private long currentSample;
    
    public MusacAudioPlayer() {
    }
    
    @Override
    public boolean load(FileHandle file) {
        try {
            // Dispose previous resources
            stop();
            dispose();
            
            // Read file data
            byte[] data = file.readBytes();
            
            // Create decoder from data
            decoder = new MusacDecoder(data);
            audioSpec = decoder.getSpec();
            
            // Calculate total duration
            totalSamples = 0; // Not available in current API
            currentSample = 0;
            
            // Create audio device
            audioDevice = Gdx.audio.newAudioDevice(
                audioSpec.freq,
                audioSpec.channels == 1
            );
            
            return true;
        } catch (Exception e) {
            if (listener.get() != null) {
                listener.get().onError("Failed to load audio: " + e.getMessage());
            }
            return false;
        }
    }
    
    @Override
    public void play() {
        if (decoder == null || audioDevice == null) {
            return;
        }
        
        if (paused.get()) {
            paused.set(false);
            return;
        }
        
        if (playing.get()) {
            return;
        }
        
        playing.set(true);
        paused.set(false);
        
        playbackThread = new Thread(() -> {
            short[] buffer = new short[BUFFER_SIZE];
            
            while (playing.get() && !Thread.currentThread().isInterrupted()) {
                try {
                    if (paused.get()) {
                        Thread.sleep(10);
                        continue;
                    }
                    
                    // Decode audio data
                    short[] decoded = decoder.decodeShort(BUFFER_SIZE);
                    int samplesRead = decoded != null ? decoded.length : 0;
                    if (samplesRead > 0) {
                        System.arraycopy(decoded, 0, buffer, 0, Math.min(samplesRead, buffer.length));
                    }
                    
                    if (samplesRead <= 0) {
                        // End of stream
                        playing.set(false);
                        if (listener.get() != null) {
                            Gdx.app.postRunnable(() -> listener.get().onComplete());
                        }
                        break;
                    }
                    
                    // Apply volume
                    if (volume != 1.0f) {
                        for (int i = 0; i < samplesRead; i++) {
                            buffer[i] = (short)(buffer[i] * volume);
                        }
                    }
                    
                    // Write to audio device
                    audioDevice.writeSamples(buffer, 0, samplesRead);
                    
                    // Update position
                    currentSample += samplesRead / audioSpec.channels;
                    
                    // Notify progress
                    if (listener.get() != null && totalSamples > 0) {
                        float position = (float) currentSample / audioSpec.freq;
                        float duration = (float) totalSamples / audioSpec.freq;
                        Gdx.app.postRunnable(() -> listener.get().onProgress(position, duration));
                    }
                    
                } catch (Exception e) {
                    playing.set(false);
                    if (listener.get() != null) {
                        Gdx.app.postRunnable(() -> listener.get().onError("Playback error: " + e.getMessage()));
                    }
                    break;
                }
            }
        });
        
        playbackThread.setName("MusacAudioPlayer");
        playbackThread.start();
    }
    
    @Override
    public void pause() {
        paused.set(true);
    }
    
    @Override
    public void stop() {
        playing.set(false);
        paused.set(false);
        
        if (playbackThread != null) {
            try {
                playbackThread.interrupt();
                playbackThread.join(1000);
            } catch (InterruptedException e) {
                // Ignore
            }
            playbackThread = null;
        }
        
        if (decoder != null) {
            decoder.rewind();
            currentSample = 0;
        }
    }
    
    @Override
    public void seek(float seconds) {
        if (decoder != null) {
            long sample = (long)(seconds * audioSpec.freq);
            decoder.seek(sample);
            currentSample = sample;
        }
    }
    
    @Override
    public float getPosition() {
        if (audioSpec != null) {
            return (float) currentSample / audioSpec.freq;
        }
        return 0;
    }
    
    @Override
    public float getDuration() {
        if (audioSpec != null && totalSamples > 0) {
            return (float) totalSamples / audioSpec.freq;
        }
        return 0;
    }
    
    @Override
    public void setVolume(float volume) {
        this.volume = Math.max(0, Math.min(1, volume));
    }
    
    @Override
    public float getVolume() {
        return volume;
    }
    
    @Override
    public boolean isPlaying() {
        return playing.get() && !paused.get();
    }
    
    @Override
    public boolean isPaused() {
        return paused.get();
    }
    
    @Override
    public void setListener(PlaybackListener listener) {
        this.listener.set(listener);
    }
    
    @Override
    public void dispose() {
        stop();
        
        if (audioDevice != null) {
            audioDevice.dispose();
            audioDevice = null;
        }
        
        if (decoder != null) {
            decoder.close();
            decoder = null;
        }
    }
}