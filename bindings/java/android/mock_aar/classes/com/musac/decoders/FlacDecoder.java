package com.musac.decoders;

import com.musac.MusacDecoder;
import java.io.IOException;

/**
 * Decoder for FLAC audio files
 */
public class FlacDecoder extends MusacDecoder {
    
    public FlacDecoder(byte[] data) throws IOException {
        super(data);
        String format = getName();
        if (!format.contains("FLAC")) {
            close();
            throw new IOException("Not a FLAC file: " + format);
        }
    }
    
    /**
     * Check if data is a FLAC file
     */
    public static boolean isFlac(byte[] data) {
        if (data == null || data.length < 4) return false;
        
        // Check for fLaC header
        return data[0] == 'f' && data[1] == 'L' && 
               data[2] == 'a' && data[3] == 'C';
    }
}