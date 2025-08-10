package com.musac.decoders;

import com.musac.MusacDecoder;
import java.io.IOException;

/**
 * Decoder for MIDI/MUS/XMI/HMI/HMP sequence files
 */
public class MidiDecoder extends MusacDecoder {
    
    public MidiDecoder(byte[] data) throws IOException {
        super(data);
        String format = getName();
        if (!format.contains("MIDI") && !format.contains("Sequence")) {
            close();
            throw new IOException("Not a MIDI sequence file: " + format);
        }
    }
    
    /**
     * Check if data is a MIDI file
     */
    public static boolean isMidi(byte[] data) {
        if (data == null || data.length < 4) return false;
        
        // Check for MThd header
        return data[0] == 'M' && data[1] == 'T' && 
               data[2] == 'h' && data[3] == 'd';
    }
    
    /**
     * Check if data is a MUS file (Doom)
     */
    public static boolean isMus(byte[] data) {
        if (data == null || data.length < 4) return false;
        
        // Check for MUS\x1A signature
        return data[0] == 'M' && data[1] == 'U' && 
               data[2] == 'S' && data[3] == 0x1A;
    }
    
    /**
     * Check if data is an XMI file
     */
    public static boolean isXmi(byte[] data) {
        if (data == null || data.length < 4) return false;
        
        // Check for FORM header (IFF format)
        return data[0] == 'F' && data[1] == 'O' && 
               data[2] == 'R' && data[3] == 'M';
    }
    
    /**
     * Check if data is an HMI/HMP file
     */
    public static boolean isHmi(byte[] data) {
        if (data == null || data.length < 4) return false;
        
        // Check for HMI-MIDISONG or HMIMIDIP
        if (data.length >= 12) {
            String sig = new String(data, 0, 12);
            if (sig.startsWith("HMI-MIDISONG")) return true;
        }
        if (data.length >= 8) {
            String sig = new String(data, 0, 8);
            if (sig.equals("HMIMIDIP")) return true;
        }
        return false;
    }
    
    /**
     * Check if data is any MIDI sequence format
     */
    public static boolean isMidiSequence(byte[] data) {
        return isMidi(data) || isMus(data) || isXmi(data) || isHmi(data);
    }
}