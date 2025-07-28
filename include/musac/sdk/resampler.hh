// This is copyrighted software. More information is at the end of this file.
#ifndef MUSAC_RESAMPLER
#define MUSAC_RESAMPLER

#include <cstddef>
#include <memory>

namespace musac {
    class decoder;
    /*!
     * \brief Abstract base class for audio resamplers.
     *
     * This class receives audio from an Decoder and resamples it to the requested sample rate.
     */
    class resampler {
        public:
            /*!
             * \brief Constructs an audio resampler.
             */
            resampler();

            virtual ~resampler();

            resampler(const resampler&) = delete;
            auto operator=(const resampler&) -> resampler& = delete;

            /*! \brief Sets the decoder that is to be used as source.
             *
             * \param decoder
             *  The decoder to use as source. Must not be null.
             */
            void set_decoder(std::shared_ptr <decoder> decoder);

            /*! \brief Sets the target sample rate, channels and chuck size.
             *
             * \param dst_rate Wanted sample rate.
             *
             * \param channels Wanted amount of channels.
             *
             * \param chunk_size
             *  Specifies how many samples per channel to resample at most in each call to the resample()
             *  function. It is recommended to set this to the same value that was used as buffer size in
             *  the call to Aulib::init().
             */
            int set_spec(unsigned int dst_rate, unsigned int channels, unsigned int chunk_size);

            [[nodiscard]] unsigned int get_current_rate() const;
            [[nodiscard]] unsigned int get_current_channels() const;
            [[nodiscard]] unsigned int get_current_chunk_size() const;

            /*! \brief Fills an output buffer with resampled audio samples.
             *
             * \param dst Output buffer.
             *
             * \param dst_len Size of output buffer (amount of elements, not size in bytes.)
             *
             * \return The amount of samples that were stored in the buffer. This can be smaller than
             *         'dstLen' if the decoder has no more samples left.
             */
            std::size_t resample(float dst[], std::size_t dst_len);

            /*! \brief Discards any samples that have not yet been retrieved with resample().
             *
             * This is especially useful after seeking the decoder to a different position and you want
             * resample() to immediately give you samples from the new position rather than the ones from
             * the old position that were previously resampled but not yet retrieved.
             */
            void discard_pending_samples();

        protected:
            /*! \brief Change sample rate and amount of channels.
             *
             * This function must be implemented when subclassing. It is used to notify subclasses about
             * changes in source and target sample rates, as well as the amount of channels in the audio.
             *
             * \param dst_rate Target sample rate (rate being resampled to.)
             *
             * \param src_rate Source sample rate (rate being resampled from.)
             *
             * \param channels Amount of channels in both the source as well as the target audio buffers.
             */
            virtual int adjust_for_output_spec(unsigned int dst_rate, unsigned int src_rate, unsigned int channels) = 0;

            /*! This function must be implemented when subclassing. It must resample
             * the audio contained in 'src' containing 'srcLen' samples, and store the
             * resulting samples in 'dst', which has a capacity of at most 'dstLen'
             * samples.
             *
             * The 'src' buffer contains audio in either mono or stereo. Stereo is
             * stored in interleaved format.
             *
             * The source and target sample rates, as well as the amount of channels
             * that are to be used must be those that were specified in the last call
             * to the adjustForOutputSpec() function.
             *
             * 'dstLen' and 'srcLen' are both input as well as output parameters. The
             * function must set 'dstLen' to the amount of samples that were actually
             * stored in 'dst', and 'srcLen' to the amount of samples that were
             * actually used from 'src'. For example, if in the following call:
             *
             *      dstLen = 200;
             *      srcLen = 100;
             *      doResampling(dst, src, dstLen, srcLen);
             *
             * the function resamples 98 samples from 'src', resulting in 196 samples
             * which are stored in 'dst', the function must set 'srcLen' to 98 and
             * 'dstLen' to 196.
             *
             * So when implementing this function, you do not need to worry about using
             * up all the available samples in 'src'. Simply resample as much audio
             * from 'src' as you can in order to fill 'dst' as much as possible, and if
             * there's anything left at the end that cannot be resampled, simply ignore
             * it.
             */
            virtual void do_resampling(float dst[], const float src[], std::size_t& dst_len, std::size_t& src_len) = 0;

            /*! \brief Discard any internally held samples.
             *
             * This function must be implemented when subclassing. It should discard any internally held
             * samples. Note that even if you don't actually buffer any samples in your subclass but are
             * using some external resampling library that you delegate resampling to, that external
             * resampler might be holding samples in an internal buffer. Those will need to be discarded as
             * well.
             *
             * If none of the above applies, this can be implemented as an empty function.
             */
            virtual void do_discard_pending_samples() = 0;

        private:
            struct impl;
            std::unique_ptr <impl> m_pimpl;
    };
}
#endif
