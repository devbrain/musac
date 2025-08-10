package org.acme;

import com.badlogic.gdx.ApplicationAdapter;
import com.badlogic.gdx.Gdx;
import com.badlogic.gdx.Input;
import com.badlogic.gdx.audio.Music;
import com.badlogic.gdx.files.FileHandle;
import com.badlogic.gdx.graphics.Color;
import com.badlogic.gdx.graphics.g2d.BitmapFont;
import com.badlogic.gdx.graphics.g2d.SpriteBatch;
import com.badlogic.gdx.utils.ScreenUtils;
import org.acme.audio.MusacAudio;

/** Simple test for Musac audio integration with libGDX */
public class SimpleMusacTest extends ApplicationAdapter {
    private SpriteBatch batch;
    private BitmapFont font;
    private Music currentMusic;
    private String statusText = "Press SPACE to load test audio, P to play, S to stop";
    
    @Override
    public void create() {
        batch = new SpriteBatch();
        font = new BitmapFont();
        font.setColor(Color.WHITE);
        
        Gdx.app.log("SimpleMusacTest", "Musac support: " + MusacAudio.isMusacAvailable());
        Gdx.app.log("SimpleMusacTest", "Supported formats: " + MusacAudio.getSupportedFormats());
    }
    
    @Override
    public void render() {
        // Handle input
        if (Gdx.input.isKeyJustPressed(Input.Keys.SPACE)) {
            loadTestAudio();
        }
        if (Gdx.input.isKeyJustPressed(Input.Keys.P)) {
            playAudio();
        }
        if (Gdx.input.isKeyJustPressed(Input.Keys.S)) {
            stopAudio();
        }
        if (Gdx.input.isKeyJustPressed(Input.Keys.ESCAPE)) {
            Gdx.app.exit();
        }
        
        // Render
        ScreenUtils.clear(0.2f, 0.2f, 0.25f, 1f);
        
        batch.begin();
        font.draw(batch, "Musac Audio Test", 20, Gdx.graphics.getHeight() - 20);
        font.draw(batch, statusText, 20, Gdx.graphics.getHeight() - 50);
        font.draw(batch, "Controls:", 20, Gdx.graphics.getHeight() - 100);
        font.draw(batch, "  SPACE - Load test audio", 20, Gdx.graphics.getHeight() - 120);
        font.draw(batch, "  P - Play", 20, Gdx.graphics.getHeight() - 140);
        font.draw(batch, "  S - Stop", 20, Gdx.graphics.getHeight() - 160);
        font.draw(batch, "  ESC - Exit", 20, Gdx.graphics.getHeight() - 180);
        
        if (currentMusic != null) {
            String playStatus = currentMusic.isPlaying() ? "Playing" : "Stopped";
            font.draw(batch, "Status: " + playStatus, 20, Gdx.graphics.getHeight() - 220);
            font.draw(batch, "Position: " + String.format("%.1f", currentMusic.getPosition()), 20, Gdx.graphics.getHeight() - 240);
        }
        
        batch.end();
    }
    
    private void loadTestAudio() {
        try {
            // Try to load any audio file that exists
            String[] testFiles = {"test.ogg", "test.wav", "test.mp3", "libgdx.png"}; // libgdx.png exists
            
            FileHandle audioFile = null;
            for (String filename : testFiles) {
                FileHandle file = Gdx.files.internal(filename);
                if (file.exists()) {
                    audioFile = file;
                    Gdx.app.log("SimpleMusacTest", "Found file: " + filename);
                    break;
                }
            }
            
            if (audioFile == null) {
                // Create a simple test tone programmatically if no file found
                statusText = "No audio files found. Using libGDX fallback.";
                // For now, just use standard libGDX
                return;
            }
            
            // Dispose previous music
            if (currentMusic != null) {
                currentMusic.dispose();
            }
            
            // Load using MusacAudio (will fallback to libGDX if needed)
            currentMusic = MusacAudio.newMusic(audioFile);
            statusText = "Loaded: " + audioFile.name();
            
            // Set completion listener
            currentMusic.setOnCompletionListener(new Music.OnCompletionListener() {
                @Override
                public void onCompletion(Music music) {
                    statusText = "Playback complete";
                }
            });
            
        } catch (Exception e) {
            statusText = "Error: " + e.getMessage();
            Gdx.app.error("SimpleMusacTest", "Failed to load audio", e);
        }
    }
    
    private void playAudio() {
        if (currentMusic != null) {
            currentMusic.play();
            statusText = "Playing...";
        } else {
            statusText = "No audio loaded";
        }
    }
    
    private void stopAudio() {
        if (currentMusic != null) {
            currentMusic.stop();
            statusText = "Stopped";
        }
    }
    
    @Override
    public void dispose() {
        if (currentMusic != null) {
            currentMusic.dispose();
        }
        if (font != null) {
            font.dispose();
        }
        if (batch != null) {
            batch.dispose();
        }
    }
}