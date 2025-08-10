package org.acme.audio;

import com.badlogic.gdx.audio.Music;
import com.badlogic.gdx.audio.AudioDevice;
import com.badlogic.gdx.Gdx;
import com.musac.MusacDecoder;
import com.musac.AudioSpec;

/**
 * Implementation of libGDX Music interface using Musac codecs.
 * This allows musac to be used transparently through standard libGDX APIs.
 */
public class MusacMusic implements Music {
    private static final int BUFFER_SIZE = 4096;
    
    private MusacDecoder decoder;
    private AudioDevice audioDevice;
    private Thread playbackThread;
    
    private volatile boolean isPlaying = false;
    private volatile boolean isPaused = false;
    private volatile boolean isLooping = false;
    private float volume = 1.0f;
    private float pan = 0.0f;
    private OnCompletionListener completionListener;
    
    private AudioSpec audioSpec;
    private long totalSamples;
    private long currentSample;
    private final Object lock = new Object();
    
    public MusacMusic(byte[] audioData) {
        try {
            decoder = new MusacDecoder(audioData);
            audioSpec = decoder.getSpec();
            totalSamples = 0; // Not available in current API
            currentSample = 0;
            
            // Create libGDX audio device
            audioDevice = Gdx.audio.newAudioDevice(
                audioSpec.freq,
                audioSpec.channels == 1
            );
        } catch (Exception e) {
            throw new RuntimeException("Failed to initialize MusacMusic", e);
        }
    }
    
    @Override
    public void play() {
        synchronized (lock) {
            if (isPlaying && !isPaused) {
                return; // Already playing
            }
            
            if (isPaused) {
                isPaused = false;
                lock.notifyAll();
                return;
            }
            
            isPlaying = true;
            isPaused = false;
            
            playbackThread = new Thread(() -> {
                short[] buffer = new short[BUFFER_SIZE];
                
                while (isPlaying) {
                    synchronized (lock) {
                        while (isPaused && isPlaying) {
                            try {
                                lock.wait();
                            } catch (InterruptedException e) {
                                Thread.currentThread().interrupt();
                                break;
                            }
                        }
                    }
                    
                    if (!isPlaying) break;
                    
                    try {
                        short[] decoded = decoder.decodeShort(BUFFER_SIZE);
                        int samplesRead = decoded != null ? decoded.length : 0;
                        if (samplesRead > 0) {
                            System.arraycopy(decoded, 0, buffer, 0, Math.min(samplesRead, buffer.length));
                        }
                        
                        if (samplesRead <= 0) {
                            // End of stream
                            if (isLooping) {
                                decoder.rewind();
                                currentSample = 0;
                                continue;
                            } else {
                                isPlaying = false;
                                if (completionListener != null) {
                                    Gdx.app.postRunnable(() -> completionListener.onCompletion(this));
                                }
                                break;
                            }
                        }
                        
                        // Apply volume and pan
                        applyVolumeAndPan(buffer, samplesRead);
                        
                        // Write to audio device
                        audioDevice.writeSamples(buffer, 0, samplesRead);
                        
                        // Update position
                        currentSample += samplesRead / audioSpec.channels;
                        
                    } catch (Exception e) {
                        isPlaying = false;
                        break;
                    }
                }
            });
            
            playbackThread.setName("MusacMusic-Playback");
            playbackThread.setDaemon(true);
            playbackThread.start();
        }
    }
    
    private void applyVolumeAndPan(short[] buffer, int samples) {
        if (volume == 1.0f && pan == 0.0f) {
            return; // No modification needed
        }
        
        if (audioSpec.channels == 2) {
            // Stereo: apply volume and pan
            float leftVolume = volume * Math.min(1.0f, 1.0f - pan);
            float rightVolume = volume * Math.min(1.0f, 1.0f + pan);
            
            for (int i = 0; i < samples; i += 2) {
                buffer[i] = (short)(buffer[i] * leftVolume);
                buffer[i + 1] = (short)(buffer[i + 1] * rightVolume);
            }
        } else {
            // Mono: just apply volume
            for (int i = 0; i < samples; i++) {
                buffer[i] = (short)(buffer[i] * volume);
            }
        }
    }
    
    @Override
    public void pause() {
        synchronized (lock) {
            if (isPlaying && !isPaused) {
                isPaused = true;
            }
        }
    }
    
    @Override
    public void stop() {
        synchronized (lock) {
            isPlaying = false;
            isPaused = false;
            lock.notifyAll();
        }
        
        if (playbackThread != null) {
            try {
                playbackThread.join(1000);
            } catch (InterruptedException e) {
                Thread.currentThread().interrupt();
            }
            playbackThread = null;
        }
        
        if (decoder != null) {
            decoder.rewind();
            currentSample = 0;
        }
    }
    
    @Override
    public boolean isPlaying() {
        return isPlaying && !isPaused;
    }
    
    @Override
    public void setLooping(boolean isLooping) {
        this.isLooping = isLooping;
    }
    
    @Override
    public boolean isLooping() {
        return isLooping;
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
    public void setPan(float pan, float volume) {
        this.pan = Math.max(-1, Math.min(1, pan));
        this.volume = Math.max(0, Math.min(1, volume));
    }
    
    @Override
    public void setPosition(float position) {
        if (decoder != null) {
            long sample = (long)(position * audioSpec.freq);
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
    
    @Override
    public void setOnCompletionListener(OnCompletionListener listener) {
        this.completionListener = listener;
    }
}