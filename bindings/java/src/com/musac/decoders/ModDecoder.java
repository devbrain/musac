package com.musac.decoders;

import com.musac.MusacDecoder;
import java.io.IOException;

/**
 * Decoder for MOD/S3M/XM/IT tracker music files
 */
public class ModDecoder extends MusacDecoder {
    
    public ModDecoder(byte[] data) throws IOException {
        super(data);
        String format = getName();
        if (!format.contains("MOD") && !format.contains("S3M") && 
            !format.contains("XM") && !format.contains("IT")) {
            close();
            throw new IOException("Not a tracker music file: " + format);
        }
    }
    
    /**
     * Check if data is a MOD file
     */
    public static boolean isMod(byte[] data) {
        if (data == null || data.length < 1084) return false;
        
        // Check for MOD signature at offset 1080
        String sig = new String(data, 1080, 4);
        return sig.equals("M.K.") || sig.equals("M!K!") || 
               sig.equals("4CHN") || sig.equals("8CHN");
    }
    
    /**
     * Check if data is an S3M file
     */
    public static boolean isS3m(byte[] data) {
        if (data == null || data.length < 48) return false;
        
        // Check for SCRM signature at offset 44
        return data[44] == 'S' && data[45] == 'C' && 
               data[46] == 'R' && data[47] == 'M';
    }
    
    /**
     * Check if data is an XM file
     */
    public static boolean isXm(byte[] data) {
        if (data == null || data.length < 17) return false;
        
        // Check for Extended Module signature
        String sig = new String(data, 0, 17);
        return sig.equals("Extended Module: ");
    }
    
    /**
     * Check if data is an IT file
     */
    public static boolean isIt(byte[] data) {
        if (data == null || data.length < 4) return false;
        
        // Check for IMPM signature
        return data[0] == 'I' && data[1] == 'M' && 
               data[2] == 'P' && data[3] == 'M';
    }
    
    /**
     * Check if data is any tracker format
     */
    public static boolean isTrackerFormat(byte[] data) {
        return isMod(data) || isS3m(data) || isXm(data) || isIt(data);
    }
}