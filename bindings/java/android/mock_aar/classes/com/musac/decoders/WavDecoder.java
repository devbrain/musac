package com.musac.decoders;

import com.musac.MusacDecoder;
import java.io.IOException;

/**
 * Decoder for WAV audio files
 */
public class WavDecoder extends MusacDecoder {
    
    public WavDecoder(byte[] data) throws IOException {
        super(data);
        String format = getName();
        if (!format.contains("WAV")) {
            close();
            throw new IOException("Not a WAV file: " + format);
        }
    }
    
    /**
     * Check if data is a WAV file
     */
    public static boolean isWav(byte[] data) {
        if (data == null || data.length < 12) return false;
        
        // Check for RIFF header
        return data[0] == 'R' && data[1] == 'I' && 
               data[2] == 'F' && data[3] == 'F' &&
               data[8] == 'W' && data[9] == 'A' && 
               data[10] == 'V' && data[11] == 'E';
    }
}