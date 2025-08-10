package com.musac;

import com.musac.decoders.*;
import java.io.IOException;
import java.io.File;
import java.nio.file.Files;
import java.nio.file.Path;

/**
 * Factory for creating appropriate decoders based on file format
 */
public class MusacDecoderFactory {
    
    /**
     * Create a decoder from byte array, auto-detecting format
     * @param data Audio file data
     * @return Appropriate decoder for the format
     * @throws IOException if format cannot be detected or decoded
     */
    public static MusacDecoder create(byte[] data) throws IOException {
        if (data == null || data.length == 0) {
            throw new IOException("Empty or null data");
        }
        
        // Try format-specific decoders first for better type safety
        if (WavDecoder.isWav(data)) {
            return new WavDecoder(data);
        } else if (Mp3Decoder.isMp3(data)) {
            return new Mp3Decoder(data);
        } else if (OggDecoder.isOgg(data)) {
            return new OggDecoder(data);
        } else if (FlacDecoder.isFlac(data)) {
            return new FlacDecoder(data);
        } else if (ModDecoder.isTrackerFormat(data)) {
            return new ModDecoder(data);
        } else if (MidiDecoder.isMidiSequence(data)) {
            return new MidiDecoder(data);
        } else if (VgmDecoder.isVgm(data) || VgmDecoder.isVgz(data)) {
            return new VgmDecoder(data);
        }
        
        // Fall back to generic decoder with auto-detection
        return new MusacDecoder(data);
    }
    
    /**
     * Create a decoder from a file
     * @param file Audio file
     * @return Appropriate decoder for the format
     * @throws IOException if file cannot be read or decoded
     */
    public static MusacDecoder create(File file) throws IOException {
        byte[] data = Files.readAllBytes(file.toPath());
        return create(data);
    }
    
    /**
     * Create a decoder from a file path
     * @param path Audio file path
     * @return Appropriate decoder for the format
     * @throws IOException if file cannot be read or decoded
     */
    public static MusacDecoder create(Path path) throws IOException {
        byte[] data = Files.readAllBytes(path);
        return create(data);
    }
    
    /**
     * Create a decoder from a file path string
     * @param filename Audio file path
     * @return Appropriate decoder for the format
     * @throws IOException if file cannot be read or decoded
     */
    public static MusacDecoder create(String filename) throws IOException {
        return create(new File(filename));
    }
    
    /**
     * Create a decoder based on file extension
     * @param data Audio file data
     * @param extension File extension (without dot)
     * @return Appropriate decoder for the format
     * @throws IOException if format cannot be decoded
     */
    public static MusacDecoder createByExtension(byte[] data, String extension) throws IOException {
        if (extension == null) {
            return create(data);
        }
        
        extension = extension.toLowerCase();
        if (extension.startsWith(".")) {
            extension = extension.substring(1);
        }
        
        switch (extension) {
            case "wav":
                return new WavDecoder(data);
            case "mp3":
                return new Mp3Decoder(data);
            case "ogg":
            case "oga":
                return new OggDecoder(data);
            case "flac":
                return new FlacDecoder(data);
            case "mod":
            case "s3m":
            case "xm":
            case "it":
                return new ModDecoder(data);
            case "mid":
            case "midi":
            case "mus":
            case "xmi":
            case "hmi":
            case "hmp":
                return new MidiDecoder(data);
            case "vgm":
            case "vgz":
                return new VgmDecoder(data);
            default:
                // Try auto-detection
                return create(data);
        }
    }
    
    /**
     * Get the file extension from a filename
     * @param filename The filename
     * @return Extension without dot, or null if no extension
     */
    public static String getExtension(String filename) {
        if (filename == null) return null;
        int lastDot = filename.lastIndexOf('.');
        if (lastDot < 0 || lastDot == filename.length() - 1) {
            return null;
        }
        return filename.substring(lastDot + 1).toLowerCase();
    }
}