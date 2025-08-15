/**
 * @file resampler.hh
 * @brief Audio resampling interface
 * @ingroup sdk_resampling
 */

// This is copyrighted software. More information is at the end of this file.
#ifndef MUSAC_RESAMPLER
#define MUSAC_RESAMPLER

#include <cstddef>
#include <memory>
#include <musac/sdk/export_musac_sdk.h>
#include <musac/sdk/types.hh>
namespace musac {
    class decoder;
    
    /**
     * @class resampler
     * @brief Abstract base class for audio resamplers
     * @ingroup sdk_resampling
     * 
     * The resampler class provides high-quality sample rate conversion
     * for audio streams. It sits between decoders and the audio output,
     * converting audio to match the device's sample rate.
     * 
     * ## Architecture
     * 
     * Resamplers work in a pipeline:
     * 1. Decoder produces audio at original sample rate
     * 2. Resampler converts to device sample rate
     * 3. Audio device plays resampled audio
     * 
     * ## Quality vs Performance
     * 
     * Different resampler implementations offer various tradeoffs:
     * - **Linear**: Fast but lower quality (good for real-time)
     * - **Cubic**: Good balance of quality and performance
     * - **Sinc**: Highest quality but computationally expensive
     * 
     * ## Implementation Guide
     * 
     * To create a custom resampler:
     * 
     * @code
     * class my_resampler : public resampler {
     * protected:
     *     int adjust_for_output_spec(sample_rate_t dst_rate, 
     *                               sample_rate_t src_rate,
     *                               channels_t channels) override {
     *         // Initialize resampling parameters
     *         m_ratio = static_cast<double>(dst_rate) / src_rate;
     *         return 0;
     *     }
     *     
     *     void do_resampling(float dst[], const float src[],
     *                       size_t& dst_len, size_t& src_len) override {
     *         // Perform actual resampling
     *         // Update dst_len and src_len with actual counts
     *     }
     *     
     *     void do_discard_pending_samples() override {
     *         // Clear internal buffers
     *     }
     * };
     * @endcode
     * 
     * @see decoder, audio_source, audio_converter
     */
    class MUSAC_SDK_EXPORT resampler {
        public:
            /**
             * @brief Constructor
             */
            resampler();

            /**
             * @brief Virtual destructor
             */
            virtual ~resampler();

            resampler(const resampler&) = delete;
            auto operator=(const resampler&) -> resampler& = delete;

            /**
             * @brief Set the source decoder
             * @param decoder Decoder to use as audio source (must not be null)
             * 
             * The resampler will read audio from this decoder and convert
             * its sample rate to match the output specification.
             */
            void set_decoder(std::shared_ptr <decoder> decoder);

            /**
             * @brief Configure output specification
             * @param dst_rate Target sample rate in Hz
             * @param channels Number of output channels
             * @param chunk_size Maximum samples per channel to process per call
             * @return 0 on success, negative on error
             * 
             * Sets the target sample rate and channel configuration for
             * resampling. The chunk_size should match your audio buffer
             * size for optimal performance.
             * 
             * @note chunk_size is per channel, so stereo with chunk_size=512
             *       means up to 1024 samples total
             */
            int set_spec(sample_rate_t dst_rate, channels_t channels, size_t chunk_size);

            /**
             * @brief Get current sample rate
             * @return Target sample rate in Hz
             */
            [[nodiscard]] unsigned int get_current_rate() const;
            
            /**
             * @brief Get current channel count
             * @return Number of channels
             */
            [[nodiscard]] unsigned int get_current_channels() const;
            
            /**
             * @brief Get current chunk size
             * @return Maximum samples per channel per call
             */
            [[nodiscard]] unsigned int get_current_chunk_size() const;

            /**
             * @brief Resample audio data
             * @param[out] dst Output buffer for resampled audio
             * @param dst_len Buffer size in samples (total, not per channel)
             * @return Number of samples actually written
             * 
             * Reads audio from the decoder, resamples it to the target
             * sample rate, and fills the output buffer. May return fewer
             * samples than requested if the decoder reaches the end.
             * 
             * @note For stereo, samples are interleaved: L,R,L,R,...
             */
            std::size_t resample(float dst[], std::size_t dst_len);

            /**
             * @brief Discard buffered samples
             * 
             * Clears any internally buffered samples that haven't been
             * retrieved yet. Useful after seeking to ensure the next
             * resample() call returns samples from the new position.
             * 
             * @code
             * decoder->seek_to_time(30s);
             * resampler->discard_pending_samples();  // Clear old samples
             * resampler->resample(buffer, size);     // Get new samples
             * @endcode
             */
            void discard_pending_samples();

        protected:
            /**
             * @brief Configure resampling parameters
             * @param dst_rate Target sample rate (Hz)
             * @param src_rate Source sample rate (Hz)
             * @param channels Number of channels
             * @return 0 on success, negative on error
             * 
             * Called when sample rates or channel configuration changes.
             * Subclasses must implement this to initialize their resampling
             * engine with the new parameters.
             * 
             * @note Must be implemented by subclasses
             */
            virtual int adjust_for_output_spec(sample_rate_t dst_rate, sample_rate_t src_rate, channels_t channels) = 0;

            /**
             * @brief Perform actual resampling
             * @param[out] dst Destination buffer for resampled audio
             * @param[in] src Source audio buffer
             * @param[in,out] dst_len Input: buffer capacity, Output: samples written
             * @param[in,out] src_len Input: available samples, Output: samples consumed
             * 
             * The core resampling function that subclasses must implement.
             * Converts audio from source to target sample rate.
             * 
             * ## Implementation Requirements
             * 
             * - Read from src[] and write to dst[]
             * - Audio is interleaved for stereo (L,R,L,R,...)
             * - Update dst_len to actual samples written
             * - Update src_len to actual samples consumed
             * - May consume fewer samples than available
             * - May produce fewer samples than buffer size
             * 
             * ## Example
             * 
             * @code
             * void do_resampling(float dst[], const float src[],
             *                   size_t& dst_len, size_t& src_len) override {
             *     size_t src_consumed = 0;
             *     size_t dst_produced = 0;
             *     
             *     while (src_consumed < src_len && dst_produced < dst_len) {
             *         // Resample one frame
             *         dst[dst_produced++] = interpolate(src, src_consumed);
             *         src_consumed += m_step;
             *     }
             *     
             *     dst_len = dst_produced;
             *     src_len = src_consumed;
             * }
             * @endcode
             * 
             * @note Must be implemented by subclasses
             */
            virtual void do_resampling(float dst[], const float src[], std::size_t& dst_len, std::size_t& src_len) = 0;

            /**
             * @brief Clear internal buffers
             * 
             * Discards any samples held in internal buffers. Called when
             * seeking or switching audio sources.
             * 
             * If your resampler doesn't buffer samples internally, this
             * can be an empty implementation. If using an external library,
             * ensure its buffers are also cleared.
             * 
             * @note Must be implemented by subclasses
             */
            virtual void do_discard_pending_samples() = 0;

        private:
            struct impl;
            std::unique_ptr <impl> m_pimpl;
    };
}
#endif
