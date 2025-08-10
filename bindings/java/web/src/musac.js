/**
 * Musac Audio Decoder Library for Web
 * JavaScript wrapper for the WebAssembly module
 */

class MusacDecoder {
    constructor() {
        this.module = null;
        this.decoder = null;
    }

    /**
     * Initialize the Musac WASM module
     * @returns {Promise} Resolves when module is loaded
     */
    static async initialize() {
        if (MusacDecoder.modulePromise) {
            return MusacDecoder.modulePromise;
        }

        MusacDecoder.modulePromise = new Promise((resolve, reject) => {
            // Load the WASM module
            import('./musac_wasm.js').then(ModuleLoader => {
                ModuleLoader.default().then(Module => {
                    MusacDecoder.Module = Module;
                    Module.init(); // Initialize the C++ library
                    console.log('Musac WASM module loaded successfully');
                    resolve(Module);
                }).catch(reject);
            }).catch(reject);
        });

        return MusacDecoder.modulePromise;
    }

    /**
     * Detect audio format from data
     * @param {Uint8Array} data - Audio file data
     * @returns {string} Format name or empty string if unknown
     */
    static detectFormat(data) {
        if (!MusacDecoder.Module) {
            throw new Error('Musac not initialized. Call MusacDecoder.initialize() first');
        }

        const Module = MusacDecoder.Module;
        
        // Convert Uint8Array to C++ vector
        const vec = new Module.Uint8Vector();
        for (let i = 0; i < data.length; i++) {
            vec.push_back(data[i]);
        }
        
        const format = Module.detectFormat(vec);
        vec.delete(); // Clean up
        
        return format;
    }

    /**
     * Check if file extension can be decoded
     * @param {string} extension - File extension without dot
     * @returns {boolean} True if supported
     */
    static canDecodeExtension(extension) {
        if (!MusacDecoder.Module) {
            throw new Error('Musac not initialized. Call MusacDecoder.initialize() first');
        }
        
        return MusacDecoder.Module.canDecodeExtension(extension);
    }

    /**
     * Create a decoder from audio data
     * @param {Uint8Array} data - Audio file data
     * @returns {MusacDecoder} Decoder instance
     */
    static create(data) {
        if (!MusacDecoder.Module) {
            throw new Error('Musac not initialized. Call MusacDecoder.initialize() first');
        }

        const decoder = new MusacDecoder();
        decoder.module = MusacDecoder.Module;
        
        // Convert Uint8Array to C++ vector
        const vec = new decoder.module.Uint8Vector();
        for (let i = 0; i < data.length; i++) {
            vec.push_back(data[i]);
        }
        
        try {
            decoder.decoder = new decoder.module.Decoder(vec);
        } catch (e) {
            vec.delete();
            throw new Error(`Failed to create decoder: ${e}`);
        }
        
        vec.delete();
        return decoder;
    }

    /**
     * Get number of audio channels
     * @returns {number} 1 for mono, 2 for stereo
     */
    getChannels() {
        if (!this.decoder) throw new Error('Decoder not initialized');
        return this.decoder.getChannels();
    }

    /**
     * Get sample rate
     * @returns {number} Sample rate in Hz
     */
    getSampleRate() {
        if (!this.decoder) throw new Error('Decoder not initialized');
        return this.decoder.getSampleRate();
    }

    /**
     * Get decoder name/format
     * @returns {string} Decoder name
     */
    getName() {
        if (!this.decoder) throw new Error('Decoder not initialized');
        return this.decoder.getName();
    }

    /**
     * Get duration in seconds
     * @returns {number} Duration or 0 if unknown
     */
    getDuration() {
        if (!this.decoder) throw new Error('Decoder not initialized');
        return this.decoder.getDuration();
    }

    /**
     * Decode audio samples as Float32Array
     * @param {number} numSamples - Number of samples to decode
     * @returns {Float32Array} Decoded samples or empty array if end of stream
     */
    decodeFloat(numSamples) {
        if (!this.decoder) throw new Error('Decoder not initialized');
        
        const result = this.decoder.decodeFloat(numSamples);
        
        // Convert from Emscripten's val to Float32Array
        if (result && result.length > 0) {
            return new Float32Array(result);
        }
        return new Float32Array(0);
    }

    /**
     * Decode audio samples as Int16Array
     * @param {number} numSamples - Number of samples to decode
     * @returns {Int16Array} Decoded samples or empty array if end of stream
     */
    decodeInt16(numSamples) {
        if (!this.decoder) throw new Error('Decoder not initialized');
        
        const result = this.decoder.decodeInt16(numSamples);
        
        // Convert from Emscripten's val to Int16Array
        if (result && result.length > 0) {
            return new Int16Array(result);
        }
        return new Int16Array(0);
    }

    /**
     * Decode entire file as Float32Array
     * @returns {Float32Array} All decoded samples
     */
    decodeAll() {
        if (!this.decoder) throw new Error('Decoder not initialized');
        
        const result = this.decoder.decodeAllFloat();
        
        if (result && result.length > 0) {
            return new Float32Array(result);
        }
        return new Float32Array(0);
    }

    /**
     * Seek to position in seconds
     * @param {number} seconds - Position to seek to
     */
    seek(seconds) {
        if (!this.decoder) throw new Error('Decoder not initialized');
        this.decoder.seek(seconds);
    }

    /**
     * Rewind to beginning
     */
    rewind() {
        if (!this.decoder) throw new Error('Decoder not initialized');
        this.decoder.rewind();
    }

    /**
     * Clean up resources
     */
    dispose() {
        if (this.decoder) {
            this.decoder.delete();
            this.decoder = null;
        }
    }
}

// Helper class for Web Audio API integration
class MusacWebAudioHelper {
    /**
     * Create AudioBuffer from decoded audio
     * @param {AudioContext} context - Web Audio context
     * @param {Float32Array} samples - Decoded samples
     * @param {number} sampleRate - Sample rate
     * @param {number} channels - Number of channels
     * @returns {AudioBuffer} Web Audio buffer
     */
    static createAudioBuffer(context, samples, sampleRate, channels) {
        const frameCount = samples.length / channels;
        const audioBuffer = context.createBuffer(channels, frameCount, sampleRate);
        
        if (channels === 1) {
            // Mono
            audioBuffer.copyToChannel(samples, 0);
        } else {
            // Stereo - deinterleave
            const left = new Float32Array(frameCount);
            const right = new Float32Array(frameCount);
            
            for (let i = 0; i < frameCount; i++) {
                left[i] = samples[i * 2];
                right[i] = samples[i * 2 + 1];
            }
            
            audioBuffer.copyToChannel(left, 0);
            audioBuffer.copyToChannel(right, 1);
        }
        
        return audioBuffer;
    }

    /**
     * Decode and play audio file
     * @param {AudioContext} context - Web Audio context
     * @param {Uint8Array} data - Audio file data
     * @returns {Promise<AudioBufferSourceNode>} Audio source node
     */
    static async decodeAndPlay(context, data) {
        // Ensure module is loaded
        await MusacDecoder.initialize();
        
        // Create decoder
        const decoder = MusacDecoder.create(data);
        
        try {
            // Get format info
            const sampleRate = decoder.getSampleRate();
            const channels = decoder.getChannels();
            
            // Decode all samples
            const samples = decoder.decodeAll();
            
            // Create audio buffer
            const audioBuffer = MusacWebAudioHelper.createAudioBuffer(
                context, samples, sampleRate, channels
            );
            
            // Create source node
            const source = context.createBufferSource();
            source.buffer = audioBuffer;
            source.connect(context.destination);
            
            return source;
        } finally {
            decoder.dispose();
        }
    }
}

// Export for use in browser or as ES6 module
if (typeof module !== 'undefined' && module.exports) {
    module.exports = { MusacDecoder, MusacWebAudioHelper };
} else if (typeof window !== 'undefined') {
    window.MusacDecoder = MusacDecoder;
    window.MusacWebAudioHelper = MusacWebAudioHelper;
}

export { MusacDecoder, MusacWebAudioHelper };