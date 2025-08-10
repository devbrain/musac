package org.acme.audio;

import com.badlogic.gdx.Gdx;
import com.badlogic.gdx.audio.Music;
import com.badlogic.gdx.audio.Sound;
import com.badlogic.gdx.files.FileHandle;

/**
 * Drop-in replacement for Gdx.audio that uses Musac codecs.
 * Supports all standard formats plus MOD, MIDI, VGM, etc.
 */
public class MusacAudio {
    
    private static boolean isMusacAvailable = false;
    
    static {
        // Check if Musac native library is available
        try {
            Class.forName("com.musac.MusacDecoder");
            isMusacAvailable = true;
        } catch (ClassNotFoundException e) {
            // Musac not available, will fallback to libGDX
            isMusacAvailable = false;
        }
    }
    
    /**
     * Creates a new Music instance from the file.
     * If Musac is available and the format is supported, uses Musac decoder.
     * Otherwise falls back to standard libGDX implementation.
     */
    public static Music newMusic(FileHandle file) {
        String extension = file.extension().toLowerCase();
        
        // Check if we should use Musac for this format
        if (isMusacAvailable && shouldUseMusac(extension)) {
            try {
                byte[] data = file.readBytes();
                return new MusacMusic(data);
            } catch (Exception e) {
                // Fallback to libGDX if Musac fails
                Gdx.app.log("MusacAudio", "Failed to load with Musac, falling back to libGDX: " + e.getMessage());
            }
        }
        
        // Use standard libGDX audio
        return Gdx.audio.newMusic(file);
    }
    
    /**
     * Creates a new Sound instance from the file.
     * For short sounds, we still use libGDX as it's optimized for that.
     */
    public static Sound newSound(FileHandle file) {
        // For sounds, always use libGDX (it's optimized for short samples)
        return Gdx.audio.newSound(file);
    }
    
    /**
     * Determines if we should use Musac for this file format
     */
    private static boolean shouldUseMusac(String extension) {
        switch (extension) {
            // Formats that Musac handles better or exclusively
            case "mod":
            case "xm":
            case "s3m":
            case "it":
            case "mid":
            case "midi":
            case "vgm":
            case "vgz":
            case "mml":
            case "cmf":
            case "dro":
            case "imf":
                return true;
            
            // Common formats - use Musac for better quality/features
            case "wav":
            case "mp3":
            case "ogg":
            case "flac":
            case "aiff":
            case "voc":
                return true;
                
            default:
                return false;
        }
    }
    
    /**
     * Check if Musac codecs are available on this platform
     */
    public static boolean isMusacAvailable() {
        return isMusacAvailable;
    }
    
    /**
     * Get supported formats string
     */
    public static String getSupportedFormats() {
        if (isMusacAvailable) {
            return "WAV, MP3, OGG, FLAC, AIFF, VOC, MOD, XM, S3M, IT, MIDI, VGM, MML, CMF, DRO, IMF";
        } else {
            return "WAV, MP3, OGG (via libGDX)";
        }
    }
}