package com.musac.android.example;

import android.app.Activity;
import android.media.AudioAttributes;
import android.media.AudioFormat;
import android.media.AudioTrack;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;

import com.musac.*;
import com.musac.android.MusacAndroidTest;
import com.musac.gdx.MusacGdxHelper;

import java.io.IOException;

/**
 * Example Android Activity using Musac decoder library
 * 
 * This demonstrates how to decode audio files and play them
 * using Android's AudioTrack API.
 */
public class ExampleActivity extends Activity {
    private static final String TAG = "MusacExample";
    
    private TextView statusText;
    private Button playButton;
    private AudioTrack audioTrack;
    private MusacGdxHelper.StreamingDecoder decoder;
    private Thread playbackThread;
    private volatile boolean isPlaying = false;
    
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        
        // Simple UI
        setContentView(createSimpleUI());
        
        // Initialize Musac
        MusacAndroidTest.init();
        statusText.setText("Musac initialized");
        
        // Run tests
        MusacAndroidTest.runTests(this);
    }
    
    private View createSimpleUI() {
        android.widget.LinearLayout layout = new android.widget.LinearLayout(this);
        layout.setOrientation(android.widget.LinearLayout.VERTICAL);
        layout.setPadding(20, 20, 20, 20);
        
        statusText = new TextView(this);
        statusText.setText("Initializing...");
        statusText.setTextSize(18);
        layout.addView(statusText);
        
        playButton = new Button(this);
        playButton.setText("Load & Play Test Audio");
        playButton.setOnClickListener(v -> togglePlayback());
        layout.addView(playButton);
        
        TextView infoText = new TextView(this);
        infoText.setText("\nSupported formats:\n" +
            "• WAV - Uncompressed audio\n" +
            "• MP3 - MPEG Layer 3\n" +
            "• OGG - Vorbis compressed\n" +
            "• FLAC - Lossless compression\n" +
            "• MOD/S3M/XM/IT - Tracker music\n" +
            "• MIDI/MUS/XMI - Sequences\n" +
            "• VGM/VGZ - Video game music");
        infoText.setTextSize(14);
        layout.addView(infoText);
        
        return layout;
    }
    
    private void togglePlayback() {
        if (isPlaying) {
            stopPlayback();
        } else {
            startPlayback();
        }
    }
    
    private void startPlayback() {
        // In a real app, you'd load an actual audio file from assets
        // For this demo, we'll create test data
        try {
            // Create test audio data (1 second sine wave)
            byte[] testData = createTestWavFile();
            
            // Decode with Musac
            statusText.setText("Decoding audio...");
            decoder = MusacGdxHelper.createStreamingDecoder(testData);
            
            // Get audio properties
            int sampleRate = decoder.getSampleRate();
            int channels = decoder.getChannels();
            
            statusText.setText(String.format("Playing: %d Hz, %s", 
                sampleRate, channels == 2 ? "Stereo" : "Mono"));
            
            // Create AudioTrack for playback
            int channelConfig = channels == 2 ? 
                AudioFormat.CHANNEL_OUT_STEREO : 
                AudioFormat.CHANNEL_OUT_MONO;
            
            int bufferSize = AudioTrack.getMinBufferSize(
                sampleRate, channelConfig, AudioFormat.ENCODING_PCM_16BIT);
            
            audioTrack = new AudioTrack.Builder()
                .setAudioAttributes(new AudioAttributes.Builder()
                    .setUsage(AudioAttributes.USAGE_MEDIA)
                    .setContentType(AudioAttributes.CONTENT_TYPE_MUSIC)
                    .build())
                .setAudioFormat(new AudioFormat.Builder()
                    .setSampleRate(sampleRate)
                    .setChannelMask(channelConfig)
                    .setEncoding(AudioFormat.ENCODING_PCM_16BIT)
                    .build())
                .setBufferSizeInBytes(bufferSize)
                .build();
            
            // Start playback thread
            isPlaying = true;
            playButton.setText("Stop");
            
            playbackThread = new Thread(() -> {
                audioTrack.play();
                
                while (isPlaying) {
                    short[] samples = decoder.readSamplesShort();
                    if (samples == null || samples.length == 0) {
                        // End of stream, loop
                        decoder.reset();
                        continue;
                    }
                    
                    audioTrack.write(samples, 0, samples.length);
                }
                
                audioTrack.stop();
                audioTrack.release();
            });
            playbackThread.start();
            
        } catch (Exception e) {
            Log.e(TAG, "Playback failed", e);
            statusText.setText("Error: " + e.getMessage());
        }
    }
    
    private void stopPlayback() {
        isPlaying = false;
        playButton.setText("Load & Play Test Audio");
        statusText.setText("Stopped");
        
        if (playbackThread != null) {
            try {
                playbackThread.join(1000);
            } catch (InterruptedException e) {
                Log.e(TAG, "Failed to stop playback thread", e);
            }
        }
        
        if (decoder != null) {
            decoder.close();
            decoder = null;
        }
    }
    
    /**
     * Create a simple WAV file for testing
     * (1 second, 440Hz sine wave)
     */
    private byte[] createTestWavFile() {
        int sampleRate = 44100;
        int duration = 1; // seconds
        int numSamples = sampleRate * duration;
        double frequency = 440.0; // A4 note
        
        // Generate sine wave
        short[] samples = new short[numSamples];
        for (int i = 0; i < numSamples; i++) {
            double angle = 2.0 * Math.PI * frequency * i / sampleRate;
            samples[i] = (short)(Math.sin(angle) * 32767);
        }
        
        // Create WAV header and data
        return createWavBytes(samples, sampleRate);
    }
    
    private byte[] createWavBytes(short[] samples, int sampleRate) {
        int numBytes = samples.length * 2;
        byte[] wav = new byte[44 + numBytes];
        
        // RIFF header
        wav[0] = 'R'; wav[1] = 'I'; wav[2] = 'F'; wav[3] = 'F';
        writeInt(wav, 4, 36 + numBytes);
        wav[8] = 'W'; wav[9] = 'A'; wav[10] = 'V'; wav[11] = 'E';
        
        // fmt chunk
        wav[12] = 'f'; wav[13] = 'm'; wav[14] = 't'; wav[15] = ' ';
        writeInt(wav, 16, 16); // chunk size
        writeShort(wav, 20, (short)1); // PCM format
        writeShort(wav, 22, (short)1); // mono
        writeInt(wav, 24, sampleRate);
        writeInt(wav, 28, sampleRate * 2); // byte rate
        writeShort(wav, 32, (short)2); // block align
        writeShort(wav, 34, (short)16); // bits per sample
        
        // data chunk
        wav[36] = 'd'; wav[37] = 'a'; wav[38] = 't'; wav[39] = 'a';
        writeInt(wav, 40, numBytes);
        
        // Write samples
        for (int i = 0; i < samples.length; i++) {
            int idx = 44 + i * 2;
            wav[idx] = (byte)(samples[i] & 0xFF);
            wav[idx + 1] = (byte)((samples[i] >> 8) & 0xFF);
        }
        
        return wav;
    }
    
    private void writeInt(byte[] data, int offset, int value) {
        data[offset] = (byte)(value & 0xFF);
        data[offset + 1] = (byte)((value >> 8) & 0xFF);
        data[offset + 2] = (byte)((value >> 16) & 0xFF);
        data[offset + 3] = (byte)((value >> 24) & 0xFF);
    }
    
    private void writeShort(byte[] data, int offset, short value) {
        data[offset] = (byte)(value & 0xFF);
        data[offset + 1] = (byte)((value >> 8) & 0xFF);
    }
    
    @Override
    protected void onDestroy() {
        super.onDestroy();
        stopPlayback();
    }
}