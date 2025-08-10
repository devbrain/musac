package org.acme.audio;

import com.badlogic.gdx.files.FileHandle;
import com.badlogic.gdx.utils.Disposable;

/**
 * Platform-agnostic audio player interface
 */
public interface AudioPlayer extends Disposable {
    
    interface PlaybackListener {
        void onProgress(float position, float duration);
        void onComplete();
        void onError(String error);
    }
    
    boolean load(FileHandle file);
    void play();
    void pause();
    void stop();
    void seek(float seconds);
    float getPosition();
    float getDuration();
    void setVolume(float volume);
    float getVolume();
    boolean isPlaying();
    boolean isPaused();
    void setListener(PlaybackListener listener);
}