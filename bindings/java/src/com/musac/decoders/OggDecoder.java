package com.musac.decoders;

import com.musac.MusacDecoder;
import java.io.IOException;

/**
 * Decoder for OGG Vorbis audio files
 */
public class OggDecoder extends MusacDecoder {
    
    public OggDecoder(byte[] data) throws IOException {
        super(data);
        String format = getName();
        if (!format.contains("Vorbis") && !format.contains("OGG")) {
            close();
            throw new IOException("Not an OGG Vorbis file: " + format);
        }
    }
    
    /**
     * Check if data is an OGG file
     */
    public static boolean isOgg(byte[] data) {
        if (data == null || data.length < 4) return false;
        
        // Check for OggS header
        return data[0] == 'O' && data[1] == 'g' && 
               data[2] == 'g' && data[3] == 'S';
    }
}