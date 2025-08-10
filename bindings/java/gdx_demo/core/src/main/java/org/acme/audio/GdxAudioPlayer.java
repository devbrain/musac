package org.acme.audio;

import com.badlogic.gdx.Gdx;
import com.badlogic.gdx.audio.Music;
import com.badlogic.gdx.files.FileHandle;
import com.badlogic.gdx.utils.Timer;

/**
 * Cross-platform audio player using libGDX's built-in audio support
 * Works on all platforms including HTML5/GWT
 */
public class GdxAudioPlayer implements AudioPlayer {
    private Music music;
    private PlaybackListener listener;
    private Timer.Task progressTask;
    private float volume = 1.0f;
    private boolean wasPlaying = false;
    
    @Override
    public boolean load(FileHandle file) {
        try {
            // Dispose previous music
            if (music != null) {
                music.dispose();
                music = null;
            }
            
            // Stop progress updates
            if (progressTask != null) {
                progressTask.cancel();
                progressTask = null;
            }
            
            // Load new music
            music = Gdx.audio.newMusic(file);
            music.setVolume(volume);
            
            // Set completion listener
            music.setOnCompletionListener(new Music.OnCompletionListener() {
                @Override
                public void onCompletion(Music music) {
                    if (listener != null) {
                        Gdx.app.postRunnable(() -> listener.onComplete());
                    }
                }
            });
            
            // Start progress updates
            progressTask = Timer.schedule(new Timer.Task() {
                @Override
                public void run() {
                    if (music != null && music.isPlaying() && listener != null) {
                        float position = music.getPosition();
                        // For GDX Music, we can't get total duration reliably before playing
                        // So we use a workaround
                        float duration = estimateDuration();
                        Gdx.app.postRunnable(() -> listener.onProgress(position, duration));
                    }
                }
            }, 0, 0.1f); // Update every 100ms
            
            return true;
        } catch (Exception e) {
            if (listener != null) {
                listener.onError("Failed to load audio: " + e.getMessage());
            }
            return false;
        }
    }
    
    private float estimateDuration() {
        // This is a limitation of libGDX Music API
        // In a real implementation, you might want to pre-calculate this
        // or use platform-specific APIs
        return 0; // Will show as 0.0 / 0.0 in UI
    }
    
    @Override
    public void play() {
        if (music != null) {
            music.play();
            wasPlaying = true;
        }
    }
    
    @Override
    public void pause() {
        if (music != null) {
            music.pause();
            wasPlaying = false;
        }
    }
    
    @Override
    public void stop() {
        if (music != null) {
            music.stop();
            wasPlaying = false;
        }
    }
    
    @Override
    public void seek(float seconds) {
        if (music != null) {
            music.setPosition(seconds);
        }
    }
    
    @Override
    public float getPosition() {
        if (music != null) {
            return music.getPosition();
        }
        return 0;
    }
    
    @Override
    public float getDuration() {
        // libGDX doesn't provide duration for Music objects
        return estimateDuration();
    }
    
    @Override
    public void setVolume(float volume) {
        this.volume = Math.max(0, Math.min(1, volume));
        if (music != null) {
            music.setVolume(this.volume);
        }
    }
    
    @Override
    public float getVolume() {
        return volume;
    }
    
    @Override
    public boolean isPlaying() {
        return music != null && music.isPlaying();
    }
    
    @Override
    public boolean isPaused() {
        return music != null && !music.isPlaying() && wasPlaying;
    }
    
    @Override
    public void setListener(PlaybackListener listener) {
        this.listener = listener;
    }
    
    @Override
    public void dispose() {
        if (progressTask != null) {
            progressTask.cancel();
            progressTask = null;
        }
        
        if (music != null) {
            music.dispose();
            music = null;
        }
    }
}