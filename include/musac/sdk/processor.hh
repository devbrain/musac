// This is copyrighted software. More information is at the end of this file.
#pragma once

namespace musac {
    /*!
     * \brief Abstract base class for audio processors.
     *
     * A processor receives input samples, processes them and produces output samples. It can be used to
     * alter the audio produced by a decoder. Processors run after resampling (if applicable.)
     */
    class processor {
        public:
            processor();
            virtual ~processor();

            processor(const processor&) = delete;
            auto operator=(const processor&) -> processor& = delete;

            /*!
             * \brief Process input samples and write output samples.
             *
             * This function will be called from the audio thread.
             *
             * \param[out] dest Output buffer.
             * \param[in] source Input buffer.
             * \param[in] len Input and output buffer size in samples.
             */
            virtual void process(float dest[], const float source[], unsigned int len) = 0;
    };
}