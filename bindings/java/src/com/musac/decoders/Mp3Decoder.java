package com.musac.decoders;

import com.musac.MusacDecoder;
import java.io.IOException;

/**
 * Decoder for MP3 audio files
 */
public class Mp3Decoder extends MusacDecoder {
    
    public Mp3Decoder(byte[] data) throws IOException {
        super(data);
        String format = getName();
        if (!format.contains("MP3")) {
            close();
            throw new IOException("Not an MP3 file: " + format);
        }
    }
    
    /**
     * Check if data is an MP3 file
     */
    public static boolean isMp3(byte[] data) {
        if (data == null || data.length < 3) return false;
        
        // Check for ID3 tag
        if (data[0] == 'I' && data[1] == 'D' && data[2] == '3') {
            return true;
        }
        
        // Check for MP3 sync word (11 bits set)
        if (data.length >= 2) {
            int sync = ((data[0] & 0xFF) << 8) | (data[1] & 0xFF);
            return (sync & 0xFFE0) == 0xFFE0;
        }
        
        return false;
    }
}