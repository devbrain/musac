/**
 * @file processor.hh
 * @brief Audio processing and effects interface
 * @ingroup sdk_processing
 */

// This is copyrighted software. More information is at the end of this file.
#pragma once
#include <musac/sdk/export_musac_sdk.h>
namespace musac {
    
    /**
     * @class processor
     * @brief Abstract base class for audio processors and effects
     * @ingroup sdk_processing
     * 
     * The processor class provides an interface for implementing audio
     * effects and processors that modify audio samples in real-time.
     * Processors can be chained together to create complex effect chains.
     * 
     * ## Architecture
     * 
     * Processors operate in the audio pipeline:
     * 1. Decoder produces audio
     * 2. Resampler converts sample rate
     * 3. **Processor modifies audio** ← You are here
     * 4. Mixer combines streams
     * 5. Output to device
     * 
     * ## Real-Time Constraints
     * 
     * The process() method is called from the audio thread with strict
     * real-time requirements:
     * - **No blocking operations** (mutex, file I/O, memory allocation)
     * - **Predictable execution time** (avoid complex algorithms)
     * - **Lock-free data structures** for parameter updates
     * 
     * ## Implementing a Processor
     * 
     * @code
     * class reverb_processor : public processor {
     *     float m_decay = 0.5f;
     *     std::atomic<float> m_room_size{0.3f};  // Atomic for thread safety
     *     
     * public:
     *     void process(float dest[], const float source[], 
     *                  unsigned int len) override {
     *         float room = m_room_size.load(std::memory_order_relaxed);
     *         
     *         for (unsigned int i = 0; i < len; ++i) {
     *             // Apply reverb algorithm
     *             dest[i] = source[i] + calculate_reverb(source[i], room);
     *         }
     *     }
     *     
     *     // Called from control thread
     *     void set_room_size(float size) {
     *         m_room_size.store(size, std::memory_order_relaxed);
     *     }
     * };
     * @endcode
     * 
     * ## Common Processor Types
     * 
     * - **Filters**: Low-pass, high-pass, band-pass
     * - **Dynamics**: Compressor, limiter, gate
     * - **Time-based**: Delay, echo, reverb
     * - **Modulation**: Chorus, flanger, phaser
     * - **Distortion**: Overdrive, fuzz, bitcrusher
     * - **Utility**: Gain, pan, stereo widener
     * 
     * ## Performance Tips
     * 
     * - Process in blocks for SIMD optimization
     * - Pre-calculate coefficients outside process()
     * - Use lookup tables for expensive functions
     * - Avoid denormal numbers (use DC offset or noise)
     * 
     * @see audio_stream, audio_source
     */
    class MUSAC_SDK_EXPORT processor {
        public:
            /**
             * @brief Constructor
             */
            processor();
            
            /**
             * @brief Virtual destructor
             */
            virtual ~processor();

            processor(const processor&) = delete;
            auto operator=(const processor&) -> processor& = delete;

            /**
             * @brief Process audio samples
             * @param[out] dest Output buffer for processed samples
             * @param[in] source Input buffer with source samples
             * @param[in] len Buffer size in samples (total, not per channel)
             * 
             * Core processing function called from the audio thread.
             * Must transform input samples and write to output buffer.
             * 
             * ## Implementation Notes
             * 
             * - **Real-time safe**: No blocking or allocation allowed
             * - **In-place processing**: dest and source may be the same buffer
             * - **Stereo format**: Samples are interleaved (L,R,L,R,...)
             * - **Sample range**: Float samples typically in [-1.0, 1.0]
             * 
             * ## Example Implementation
             * 
             * @code
             * void process(float dest[], const float source[], 
             *              unsigned int len) override {
             *     // Simple gain processor
             *     for (unsigned int i = 0; i < len; ++i) {
             *         dest[i] = source[i] * m_gain;
             *         
             *         // Soft clipping to prevent distortion
             *         if (dest[i] > 1.0f) dest[i] = 1.0f;
             *         else if (dest[i] < -1.0f) dest[i] = -1.0f;
             *     }
             * }
             * @endcode
             * 
             * @warning Called from audio thread - must be real-time safe!
             */
            virtual void process(float dest[], const float source[], unsigned int len) = 0;
    };
}