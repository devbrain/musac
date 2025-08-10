package com.musac.android;

import android.content.Context;
import android.content.res.AssetManager;
import android.util.Log;

import com.musac.*;
import com.musac.decoders.*;
import com.musac.gdx.MusacGdxHelper;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;

/**
 * Android test helper for Musac library
 * 
 * This class demonstrates how to use Musac on Android,
 * including loading files from assets and decoding audio.
 */
public class MusacAndroidTest {
    private static final String TAG = "MusacAndroid";
    
    /**
     * Initialize the Musac library
     * Call this in your Application or Activity onCreate
     */
    public static void init() {
        try {
            // The native library will be loaded automatically
            MusacNative.init();
            Log.i(TAG, "Musac library initialized successfully");
        } catch (Exception e) {
            Log.e(TAG, "Failed to initialize Musac", e);
        }
    }
    
    /**
     * Load an audio file from Android assets
     * @param context Android context
     * @param assetPath Path to asset file (e.g., "music/song.ogg")
     * @return Decoded audio data
     */
    public static MusacGdxHelper.DecodedAudio loadFromAssets(Context context, String assetPath) 
            throws IOException {
        byte[] data = readAssetBytes(context, assetPath);
        return MusacGdxHelper.decodeSound(data);
    }
    
    /**
     * Create a streaming decoder from asset file
     * @param context Android context
     * @param assetPath Path to asset file
     * @return Streaming decoder
     */
    public static MusacGdxHelper.StreamingDecoder createStreamingDecoder(Context context, String assetPath) 
            throws IOException {
        byte[] data = readAssetBytes(context, assetPath);
        return MusacGdxHelper.createStreamingDecoder(data);
    }
    
    /**
     * Test all decoders with a sample file
     * @param context Android context
     */
    public static void runTests(Context context) {
        Log.i(TAG, "Running Musac Android tests...");
        
        try {
            // Test format detection
            testFormatDetection();
            
            // Test decoding from assets (if test files are present)
            testAssetDecoding(context);
            
            Log.i(TAG, "All tests passed!");
        } catch (Exception e) {
            Log.e(TAG, "Test failed", e);
        }
    }
    
    private static void testFormatDetection() {
        Log.i(TAG, "Testing format detection...");
        
        String[] formats = {"wav", "mp3", "ogg", "flac", "mod", "mid"};
        for (String format : formats) {
            boolean supported = MusacNative.canDecode(format);
            Log.d(TAG, format + " supported: " + supported);
        }
    }
    
    private static void testAssetDecoding(Context context) {
        Log.i(TAG, "Testing asset decoding...");
        
        // Try to load a test file if it exists
        // In a real app, you'd include test audio files in assets/
        String[] testFiles = {"test.wav", "test.ogg", "test.mp3"};
        
        for (String testFile : testFiles) {
            try {
                byte[] data = readAssetBytes(context, testFile);
                
                // Detect format
                String format = MusacNative.detectFormat(data);
                Log.d(TAG, testFile + " detected as: " + format);
                
                // Create decoder
                try (MusacDecoder decoder = MusacDecoderFactory.create(data)) {
                    AudioSpec spec = decoder.getSpec();
                    Log.d(TAG, String.format("  Spec: %d Hz, %d channels", 
                        spec.freq, spec.channels));
                    
                    // Decode a chunk
                    float[] samples = decoder.decodeFloat(1024);
                    Log.d(TAG, "  Decoded " + samples.length + " samples");
                }
                
            } catch (IOException e) {
                // File doesn't exist, skip
                Log.d(TAG, testFile + " not found in assets (expected)");
            }
        }
    }
    
    /**
     * Read bytes from Android asset file
     */
    private static byte[] readAssetBytes(Context context, String assetPath) throws IOException {
        AssetManager assets = context.getAssets();
        try (InputStream input = assets.open(assetPath);
             ByteArrayOutputStream output = new ByteArrayOutputStream()) {
            
            byte[] buffer = new byte[4096];
            int n;
            while ((n = input.read(buffer)) != -1) {
                output.write(buffer, 0, n);
            }
            
            return output.toByteArray();
        }
    }
    
    /**
     * Example usage in an Android Activity:
     * 
     * public class MainActivity extends AppCompatActivity {
     *     @Override
     *     protected void onCreate(Bundle savedInstanceState) {
     *         super.onCreate(savedInstanceState);
     *         
     *         // Initialize Musac
     *         MusacAndroidTest.init();
     *         
     *         // Load and play audio
     *         try {
     *             DecodedAudio audio = MusacAndroidTest.loadFromAssets(
     *                 this, "music/background.ogg"
     *             );
     *             
     *             // Use with libGDX or Android AudioTrack
     *             playAudio(audio.samples, audio.sampleRate, audio.channels);
     *             
     *         } catch (IOException e) {
     *             Log.e("MainActivity", "Failed to load audio", e);
     *         }
     *     }
     * }
     */
}