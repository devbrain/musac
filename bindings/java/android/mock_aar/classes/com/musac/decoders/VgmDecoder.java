package com.musac.decoders;

import com.musac.MusacDecoder;
import java.io.IOException;

/**
 * Decoder for VGM (Video Game Music) files
 */
public class VgmDecoder extends MusacDecoder {
    
    public VgmDecoder(byte[] data) throws IOException {
        super(data);
        String format = getName();
        if (!format.contains("VGM")) {
            close();
            throw new IOException("Not a VGM file: " + format);
        }
    }
    
    /**
     * Check if data is a VGM file
     */
    public static boolean isVgm(byte[] data) {
        if (data == null || data.length < 4) return false;
        
        // Check for "Vgm " signature
        return data[0] == 'V' && data[1] == 'g' && 
               data[2] == 'm' && data[3] == ' ';
    }
    
    /**
     * Check if data is a compressed VGZ file (gzipped VGM)
     */
    public static boolean isVgz(byte[] data) {
        if (data == null || data.length < 2) return false;
        
        // Check for gzip magic number
        return (data[0] & 0xFF) == 0x1F && (data[1] & 0xFF) == 0x8B;
    }
}