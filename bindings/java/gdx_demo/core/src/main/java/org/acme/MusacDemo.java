package org.acme;

import com.badlogic.gdx.ApplicationAdapter;
import com.badlogic.gdx.Gdx;
import com.badlogic.gdx.graphics.GL20;
import com.badlogic.gdx.graphics.Color;
import com.badlogic.gdx.graphics.Texture;
import com.badlogic.gdx.graphics.g2d.BitmapFont;
import com.badlogic.gdx.graphics.g2d.SpriteBatch;
import com.badlogic.gdx.graphics.glutils.ShapeRenderer;
import com.badlogic.gdx.scenes.scene2d.Stage;
import com.badlogic.gdx.scenes.scene2d.ui.*;
import com.badlogic.gdx.scenes.scene2d.utils.ClickListener;
import com.badlogic.gdx.scenes.scene2d.InputEvent;
import com.badlogic.gdx.utils.ScreenUtils;
import com.badlogic.gdx.utils.viewport.ScreenViewport;
import com.badlogic.gdx.files.FileHandle;
import com.badlogic.gdx.utils.Array;
import com.badlogic.gdx.audio.Music;
import org.acme.audio.MusacAudio;

/** Musac Audio Player Demo for libGDX */
public class MusacDemo extends ApplicationAdapter {
    private Stage stage;
    private Skin skin;
    private Music currentMusic;
    
    // UI Elements
    private Label titleLabel;
    private Label statusLabel;
    private Label timeLabel;
    private TextButton playButton;
    private TextButton pauseButton;
    private TextButton stopButton;
    private TextButton loadButton;
    private Slider progressSlider;
    private Slider volumeSlider;
    private SelectBox<String> fileSelector;
    private Table mainTable;
    
    // State
    private boolean isDraggingSlider = false;
    private Array<String> audioFiles;
    private float updateTimer = 0;

    @Override
    public void create() {
        // For now, skip UI and just test audio loading
        Gdx.app.log("MusacDemo", "Starting Musac Audio Demo...");
        
        // Create UI
        createUI();
        
        // List available audio files
        findAudioFiles();
        
        // Show supported formats
        Gdx.app.log("MusacDemo", "Supported formats: " + MusacAudio.getSupportedFormats());
    }
    
    private void createUI() {
        mainTable = new Table();
        mainTable.setFillParent(true);
        mainTable.pad(20);
        
        // Title
        titleLabel = new Label("Musac Audio Player Demo", skin, "default");
        titleLabel.setFontScale(1.5f);
        mainTable.add(titleLabel).colspan(3).padBottom(20).row();
        
        // File selector
        fileSelector = new SelectBox<>(skin);
        loadButton = new TextButton("Load", skin);
        loadButton.addListener(new ClickListener() {
            @Override
            public void clicked(InputEvent event, float x, float y) {
                loadSelectedFile();
            }
        });
        
        mainTable.add(new Label("File:", skin)).right().padRight(10);
        mainTable.add(fileSelector).width(300).padRight(10);
        mainTable.add(loadButton).row();
        
        // Status
        statusLabel = new Label("No file loaded", skin);
        mainTable.add(statusLabel).colspan(3).padTop(10).padBottom(10).row();
        
        // Progress slider
        progressSlider = new Slider(0, 1, 0.01f, false, skin);
        progressSlider.addListener(new ClickListener() {
            @Override
            public boolean touchDown(InputEvent event, float x, float y, int pointer, int button) {
                isDraggingSlider = true;
                return super.touchDown(event, x, y, pointer, button);
            }
            
            @Override
            public void touchUp(InputEvent event, float x, float y, int pointer, int button) {
                super.touchUp(event, x, y, pointer, button);
                isDraggingSlider = false;
                if (currentMusic != null) {
                    // Seek to position (we'll estimate duration based on file size)
                    currentMusic.setPosition(progressSlider.getValue() * 180); // Assume 3 minute max
                }
            }
        });
        
        timeLabel = new Label("0.0 / 0.0", skin);
        mainTable.add(new Label("Progress:", skin)).right().padRight(10);
        mainTable.add(progressSlider).width(300).padRight(10);
        mainTable.add(timeLabel).row();
        
        // Volume control
        volumeSlider = new Slider(0, 1, 0.01f, false, skin);
        volumeSlider.setValue(1.0f);
        volumeSlider.addListener(new ClickListener() {
            @Override
            public void touchDragged(InputEvent event, float x, float y, int pointer) {
                super.touchDragged(event, x, y, pointer);
                if (currentMusic != null) {
                    currentMusic.setVolume(volumeSlider.getValue());
                }
            }
        });
        
        Label volumeLabel = new Label("100%", skin);
        volumeSlider.addListener(new ClickListener() {
            @Override
            public void touchDragged(InputEvent event, float x, float y, int pointer) {
                super.touchDragged(event, x, y, pointer);
                volumeLabel.setText(String.format("%d%%", (int)(volumeSlider.getValue() * 100)));
            }
        });
        
        mainTable.add(new Label("Volume:", skin)).right().padRight(10);
        mainTable.add(volumeSlider).width(300).padRight(10);
        mainTable.add(volumeLabel).row();
        
        // Control buttons
        Table buttonTable = new Table();
        
        playButton = new TextButton("Play", skin);
        playButton.addListener(new ClickListener() {
            @Override
            public void clicked(InputEvent event, float x, float y) {
                if (currentMusic != null) {
                    currentMusic.play();
                    playButton.setVisible(false);
                    pauseButton.setVisible(true);
                    statusLabel.setText("Playing");
                }
            }
        });
        
        pauseButton = new TextButton("Pause", skin);
        pauseButton.setVisible(false);
        pauseButton.addListener(new ClickListener() {
            @Override
            public void clicked(InputEvent event, float x, float y) {
                if (currentMusic != null) {
                    currentMusic.pause();
                    playButton.setVisible(true);
                    pauseButton.setVisible(false);
                    statusLabel.setText("Paused");
                }
            }
        });
        
        stopButton = new TextButton("Stop", skin);
        stopButton.addListener(new ClickListener() {
            @Override
            public void clicked(InputEvent event, float x, float y) {
                if (currentMusic != null) {
                    currentMusic.stop();
                    playButton.setVisible(true);
                    pauseButton.setVisible(false);
                    progressSlider.setValue(0);
                    statusLabel.setText("Stopped");
                }
            }
        });
        
        buttonTable.add(playButton).padRight(10);
        buttonTable.add(pauseButton).padRight(10);
        buttonTable.add(stopButton);
        
        mainTable.add(buttonTable).colspan(3).padTop(20).row();
        
        // Format support info
        Label formatLabel = new Label("Supported formats: WAV, MP3, OGG, FLAC, MOD, MIDI, VGM", skin);
        formatLabel.setFontScale(0.8f);
        mainTable.add(formatLabel).colspan(3).padTop(20);
        
        stage.addActor(mainTable);
    }
    
    private void findAudioFiles() {
        audioFiles = new Array<>();
        
        // Add sample files (you can add actual audio files to assets folder)
        audioFiles.add("sample.wav");
        audioFiles.add("sample.mp3");
        audioFiles.add("sample.ogg");
        audioFiles.add("music.mod");
        audioFiles.add("test.mid");
        
        // Also check for any audio files in assets
        FileHandle assetsDir = Gdx.files.internal("");
        if (assetsDir.exists()) {
            for (FileHandle file : assetsDir.list()) {
                String name = file.name().toLowerCase();
                if (name.endsWith(".wav") || name.endsWith(".mp3") || 
                    name.endsWith(".ogg") || name.endsWith(".flac") ||
                    name.endsWith(".mod") || name.endsWith(".xm") ||
                    name.endsWith(".s3m") || name.endsWith(".it") ||
                    name.endsWith(".mid") || name.endsWith(".midi") ||
                    name.endsWith(".vgm") || name.endsWith(".vgz")) {
                    if (!audioFiles.contains(file.name(), false)) {
                        audioFiles.add(file.name());
                    }
                }
            }
        }
        
        if (audioFiles.size == 0) {
            audioFiles.add("No audio files found");
        }
        
        fileSelector.setItems(audioFiles);
    }
    
    private void loadSelectedFile() {
        String selected = fileSelector.getSelected();
        if (selected != null && !selected.equals("No audio files found")) {
            FileHandle file = Gdx.files.internal(selected);
            if (file.exists()) {
                try {
                    // Dispose previous music
                    if (currentMusic != null) {
                        currentMusic.stop();
                        currentMusic.dispose();
                    }
                    
                    // Load new music using MusacAudio (will use Musac or fallback to libGDX)
                    currentMusic = MusacAudio.newMusic(file);
                    
                    // Set completion listener
                    currentMusic.setOnCompletionListener(new Music.OnCompletionListener() {
                        @Override
                        public void onCompletion(Music music) {
                            Gdx.app.postRunnable(() -> {
                                statusLabel.setText("Playback complete");
                                playButton.setVisible(true);
                                pauseButton.setVisible(false);
                                progressSlider.setValue(0);
                            });
                        }
                    });
                    
                    statusLabel.setText("Loaded: " + selected);
                    progressSlider.setValue(0);
                    playButton.setVisible(true);
                    pauseButton.setVisible(false);
                } catch (Exception e) {
                    statusLabel.setText("Failed to load: " + e.getMessage());
                }
            } else {
                statusLabel.setText("File not found: " + selected);
            }
        }
    }

    @Override
    public void render() {
        ScreenUtils.clear(0.2f, 0.2f, 0.25f, 1f);
        
        // Update UI
        updateTimer += Gdx.graphics.getDeltaTime();
        if (updateTimer > 0.1f) { // Update every 100ms
            updateTimer = 0;
            
            // Update UI state based on music state
            if (currentMusic != null) {
                if (!isDraggingSlider && currentMusic.isPlaying()) {
                    float position = currentMusic.getPosition();
                    // Since we can't get duration from Music interface, estimate or use fixed value
                    float duration = 180; // Assume 3 minutes for demo
                    progressSlider.setValue(position / duration);
                    timeLabel.setText(String.format("%.1f / %.1f", position, duration));
                }
            }
        }
        
        stage.act(Gdx.graphics.getDeltaTime());
        stage.draw();
    }
    
    @Override
    public void resize(int width, int height) {
        stage.getViewport().update(width, height, true);
    }

    @Override
    public void dispose() {
        if (currentMusic != null) {
            currentMusic.dispose();
        }
        stage.dispose();
        skin.dispose();
    }
}
