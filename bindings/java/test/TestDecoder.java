import com.musac.*;
import java.io.*;
import java.nio.file.*;

public class TestDecoder {
    public static void main(String[] args) {
        if (args.length < 1) {
            System.err.println("Usage: java TestDecoder <audio_file>");
            System.exit(1);
        }
        
        try {
            // Load the audio file
            String filename = args[0];
            byte[] data = Files.readAllBytes(Paths.get(filename));
            System.out.println("Loaded file: " + filename + " (" + data.length + " bytes)");
            
            // Detect format
            String format = MusacNative.detectFormat(data);
            System.out.println("Detected format: " + format);
            
            // Create decoder
            MusacDecoder decoder = new MusacDecoder(data);
            System.out.println("Created decoder: " + decoder.getName());
            
            // Get audio spec
            AudioSpec spec = decoder.getSpec();
            System.out.println("Audio spec: " + spec);
            
            // Get duration
            double duration = decoder.getDuration();
            if (duration > 0) {
                System.out.println("Duration: " + duration + " seconds");
            }
            
            // Decode some samples
            System.out.println("\nDecoding samples...");
            int totalSamples = 0;
            int chunks = 0;
            
            while (true) {
                short[] samples = decoder.decodeShort(spec.samples);
                if (samples == null || samples.length == 0) {
                    break;
                }
                totalSamples += samples.length;
                chunks++;
                
                if (chunks <= 3) {
                    System.out.println("Chunk " + chunks + ": " + samples.length + " samples");
                }
            }
            
            System.out.println("\nTotal decoded: " + totalSamples + " samples in " + chunks + " chunks");
            double decodedDuration = spec.samplesToSeconds(totalSamples);
            System.out.println("Decoded duration: " + decodedDuration + " seconds");
            
            // Test seeking
            if (duration > 0) {
                System.out.println("\nTesting seek to middle...");
                decoder.seek(duration / 2);
                short[] midSamples = decoder.decodeShort(spec.samples);
                System.out.println("Decoded " + midSamples.length + " samples from middle");
            }
            
            // Clean up
            decoder.close();
            System.out.println("\nTest completed successfully!");
            
        } catch (Exception e) {
            System.err.println("Error: " + e.getMessage());
            e.printStackTrace();
            System.exit(1);
        }
    }
}