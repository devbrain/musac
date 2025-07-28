// This is copyrighted software. More information is at the end of this file.
#pragma once

#include <musac/sdk/resampler.hh>

namespace musac {
    /*!
     * \brief Speex resampler.
     */
    class resampler_speex : public resampler {
        public:
            /*!
             * \param quality
             *      Speex resampler quality level, from 0 to 10. The value is clamped
             *      if it lies outside this range. Note that the quality can be changed
             *      later on if required.
             */
            explicit resampler_speex(int quality = 5);
            ~resampler_speex() override;

            [[nodiscard]] int quality() const noexcept;
            void set_quality(int quality);

        protected:
            void do_resampling(float dst[], const float src[], std::size_t& dst_len, std::size_t& src_len) override;
            int adjust_for_output_spec(unsigned int dst_rate, unsigned int src_rate, unsigned int channels) override;
            void do_discard_pending_samples() override;

        private:
            struct impl;
            const std::unique_ptr <impl> m_pimpl;
    };
} // namespace Aulib
