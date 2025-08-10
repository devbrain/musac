package com.musac.gwt;

import com.google.gwt.core.client.JavaScriptObject;
import com.google.gwt.core.client.JsArray;
import com.google.gwt.typedarrays.shared.Float32Array;
import com.google.gwt.typedarrays.shared.Int16Array;
import com.google.gwt.typedarrays.shared.Uint8Array;

/**
 * GWT bindings for Musac WASM decoder
 * 
 * This allows libGDX HTML5 backend to use Musac decoders
 */
public class MusacGWT {
    
    /**
     * Initialize the Musac WASM module
     * Must be called before using any decoder functions
     */
    public static native void initialize() /*-{
        if (!$wnd.MusacDecoder) {
            throw new Error("MusacDecoder not loaded. Include musac.js before GWT module");
        }
        
        return $wnd.MusacDecoder.initialize();
    }-*/;
    
    /**
     * Check if Musac is initialized
     */
    public static native boolean isInitialized() /*-{
        return $wnd.MusacDecoder && $wnd.MusacDecoder.Module != null;
    }-*/;
    
    /**
     * Detect audio format from data
     */
    public static native String detectFormat(Uint8Array data) /*-{
        return $wnd.MusacDecoder.detectFormat(data);
    }-*/;
    
    /**
     * Check if extension can be decoded
     */
    public static native boolean canDecodeExtension(String extension) /*-{
        return $wnd.MusacDecoder.canDecodeExtension(extension);
    }-*/;
    
    /**
     * Native decoder handle
     */
    public static class Decoder extends JavaScriptObject {
        protected Decoder() {}
        
        /**
         * Create decoder from audio data
         */
        public static native Decoder create(Uint8Array data) /*-{
            return $wnd.MusacDecoder.create(data);
        }-*/;
        
        /**
         * Get number of channels
         */
        public final native int getChannels() /*-{
            return this.getChannels();
        }-*/;
        
        /**
         * Get sample rate
         */
        public final native int getSampleRate() /*-{
            return this.getSampleRate();
        }-*/;
        
        /**
         * Get decoder name
         */
        public final native String getName() /*-{
            return this.getName();
        }-*/;
        
        /**
         * Get duration in seconds
         */
        public final native double getDuration() /*-{
            return this.getDuration();
        }-*/;
        
        /**
         * Decode samples as float array
         */
        public final native Float32Array decodeFloat(int numSamples) /*-{
            return this.decodeFloat(numSamples);
        }-*/;
        
        /**
         * Decode samples as int16 array
         */
        public final native Int16Array decodeInt16(int numSamples) /*-{
            return this.decodeInt16(numSamples);
        }-*/;
        
        /**
         * Decode entire file
         */
        public final native Float32Array decodeAll() /*-{
            return this.decodeAll();
        }-*/;
        
        /**
         * Seek to position
         */
        public final native void seek(double seconds) /*-{
            this.seek(seconds);
        }-*/;
        
        /**
         * Rewind to beginning
         */
        public final native void rewind() /*-{
            this.rewind();
        }-*/;
        
        /**
         * Dispose decoder
         */
        public final native void dispose() /*-{
            this.dispose();
        }-*/;
    }
    
    /**
     * Helper for Web Audio API
     */
    public static class WebAudioHelper {
        
        /**
         * Decode and create Web Audio buffer
         */
        public static native JavaScriptObject createAudioBuffer(
            JavaScriptObject context, 
            Uint8Array data
        ) /*-{
            return $wnd.MusacWebAudioHelper.decodeAndPlay(context, data);
        }-*/;
    }
    
    /**
     * Convert byte array to Uint8Array
     */
    public static native Uint8Array toUint8Array(byte[] bytes) /*-{
        var array = new Uint8Array(bytes.length);
        for (var i = 0; i < bytes.length; i++) {
            array[i] = bytes[i] & 0xFF;
        }
        return array;
    }-*/;
    
    /**
     * Convert Float32Array to float array
     */
    public static float[] toFloatArray(Float32Array jsArray) {
        int length = jsArray.length();
        float[] result = new float[length];
        for (int i = 0; i < length; i++) {
            result[i] = (float) jsArray.get(i);
        }
        return result;
    }
    
    /**
     * Convert Int16Array to short array
     */
    public static short[] toShortArray(Int16Array jsArray) {
        int length = jsArray.length();
        short[] result = new short[length];
        for (int i = 0; i < length; i++) {
            result[i] = (short) jsArray.get(i);
        }
        return result;
    }
}