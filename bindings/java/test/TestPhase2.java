import com.musac.*;
import com.musac.decoders.*;
import com.musac.gdx.*;
import java.io.*;
import java.nio.file.*;

public class TestPhase2 {
    public static void main(String[] args) throws Exception {
        System.out.println("=== Musac Phase 2 Test Suite ===\n");
        
        // Test extension detection
        testExtensionDetection();
        
        // Test format-specific decoders
        testFormatSpecificDecoders();
        
        // Test factory pattern
        testFactory();
        
        // Test PCM utilities
        testPcmUtils();
        
        // Test libGDX helper
        testGdxHelper();
        
        System.out.println("\n=== All Phase 2 Tests Passed! ===");
    }
    
    static void testExtensionDetection() {
        System.out.println("Testing extension detection:");
        
        String[] supported = {"wav", "mp3", "ogg", "flac", "mod", "mid", "vgm"};
        for (String ext : supported) {
            boolean canDecode = MusacNative.canDecode(ext);
            System.out.println("  " + ext + ": " + (canDecode ? "✓" : "✗"));
            assert canDecode : "Should support " + ext;
        }
        
        String[] unsupported = {"txt", "pdf", "exe"};
        for (String ext : unsupported) {
            boolean canDecode = MusacNative.canDecode(ext);
            System.out.println("  " + ext + ": " + (!canDecode ? "✓ (correctly unsupported)" : "✗"));
            assert !canDecode : "Should not support " + ext;
        }
        
        System.out.println("Extension detection test passed!\n");
    }
    
    static void testFormatSpecificDecoders() throws Exception {
        System.out.println("Testing format-specific decoders:");
        
        // Test WAV
        byte[] wavData = Files.readAllBytes(Paths.get("../../../data/soundcard.wav"));
        assert WavDecoder.isWav(wavData) : "Should detect WAV";
        WavDecoder wav = new WavDecoder(wavData);
        System.out.println("  WAV decoder: " + wav.getName() + " ✓");
        wav.close();
        
        // Test OGG
        byte[] oggData = Files.readAllBytes(Paths.get("../../../data/punch.ogg"));
        assert OggDecoder.isOgg(oggData) : "Should detect OGG";
        OggDecoder ogg = new OggDecoder(oggData);
        System.out.println("  OGG decoder: " + ogg.getName() + " ✓");
        ogg.close();
        
        // Test MIDI
        byte[] midiData = Files.readAllBytes(Paths.get("../../../data/simon.mid"));
        assert MidiDecoder.isMidi(midiData) : "Should detect MIDI";
        MidiDecoder midi = new MidiDecoder(midiData);
        System.out.println("  MIDI decoder: " + midi.getName() + " ✓");
        midi.close();
        
        System.out.println("Format-specific decoders test passed!\n");
    }
    
    static void testFactory() throws Exception {
        System.out.println("Testing decoder factory:");
        
        // Test auto-detection
        MusacDecoder decoder1 = MusacDecoderFactory.create("../../../data/soundcard.wav");
        System.out.println("  Auto-detected WAV: " + decoder1.getName() + " ✓");
        decoder1.close();
        
        // Test by extension
        byte[] data = Files.readAllBytes(Paths.get("../../../data/punch.ogg"));
        MusacDecoder decoder2 = MusacDecoderFactory.createByExtension(data, "ogg");
        System.out.println("  By extension OGG: " + decoder2.getName() + " ✓");
        decoder2.close();
        
        // Test extension extraction
        String ext = MusacDecoderFactory.getExtension("test.mp3");
        assert "mp3".equals(ext) : "Should extract mp3";
        System.out.println("  Extension extraction: " + ext + " ✓");
        
        System.out.println("Factory test passed!\n");
    }
    
    static void testPcmUtils() {
        System.out.println("Testing PCM utilities:");
        
        // Test float to short conversion
        float[] floats = {0.0f, 0.5f, 1.0f, -0.5f, -1.0f};
        short[] shorts = PcmUtils.floatToShort(floats);
        assert shorts[0] == 0 : "0.0 should convert to 0";
        assert shorts[2] == 32767 : "1.0 should convert to 32767";
        assert shorts[4] == -32768 : "-1.0 should convert to -32768";
        System.out.println("  Float to short conversion ✓");
        
        // Test short to float conversion
        float[] backToFloat = PcmUtils.shortToFloat(shorts);
        assert Math.abs(backToFloat[1] - 0.5f) < 0.01f : "Round trip should work";
        System.out.println("  Short to float conversion ✓");
        
        // Test stereo operations
        float[] left = {0.1f, 0.2f, 0.3f};
        float[] right = {0.4f, 0.5f, 0.6f};
        float[] interleaved = PcmUtils.interleaveStereo(left, right);
        assert interleaved.length == 6 : "Should have 6 samples";
        assert interleaved[0] == 0.1f && interleaved[1] == 0.4f : "Should interleave correctly";
        System.out.println("  Stereo interleaving ✓");
        
        float[][] deinterleaved = PcmUtils.deinterleaveStereo(interleaved);
        assert deinterleaved[0][0] == 0.1f : "Should deinterleave left";
        assert deinterleaved[1][0] == 0.4f : "Should deinterleave right";
        System.out.println("  Stereo deinterleaving ✓");
        
        System.out.println("PCM utilities test passed!\n");
    }
    
    static void testGdxHelper() throws Exception {
        System.out.println("Testing libGDX helper:");
        
        // Test sound decoding (short clip)
        byte[] wavData = Files.readAllBytes(Paths.get("../../../data/soundcard.wav"));
        MusacGdxHelper.DecodedAudio audio = MusacGdxHelper.decodeSound(wavData);
        System.out.println("  Decoded sound: " + audio.sampleRate + "Hz, " + 
                         audio.channels + "ch, " + 
                         audio.getDuration() + "s ✓");
        
        // Test streaming decoder
        byte[] oggData = Files.readAllBytes(Paths.get("../../../data/punch.ogg"));
        try (MusacGdxHelper.StreamingDecoder streaming = 
             MusacGdxHelper.createStreamingDecoder(oggData)) {
            
            System.out.println("  Streaming decoder: " + 
                             streaming.getSampleRate() + "Hz, " +
                             (streaming.isStereo() ? "stereo" : "mono") + ", " +
                             streaming.getDuration() + "s");
            
            // Read a few chunks
            int chunks = 0;
            float[] chunk;
            while ((chunk = streaming.readSamples()) != null && chunks < 3) {
                System.out.println("    Chunk " + (chunks + 1) + ": " + chunk.length + " samples");
                chunks++;
            }
            
            // Test seeking
            streaming.seek(10.0f);
            chunk = streaming.readSamples();
            System.out.println("    After seek: " + (chunk != null ? chunk.length : 0) + " samples ✓");
        }
        
        System.out.println("libGDX helper test passed!\n");
    }
}