// This is copyrighted software. More information is at the end of this file.
#pragma once
#include <musac/sdk/resampler.hh>

namespace musac {
    /*!
     * \brief SDL_AudioStream resampler.
     *
     * This uses the built-in resampling functionality of SDL (through the SDL_AudioStream API) and has
     * no external dependencies. Requires at least SDL 2.0.7. Note that SDL can be built to use
     * libsamplerate instead of its own resampler. There's no way to detect whether this is the case or
     * not.
     *
     * It usually makes no sense to use this resampler, unless you have a specific need to use SDL's
     * resampler and you know that the SDL version you're running on was not built with libsamplerate
     * support. If you do want libsamplerate, then you can just use \ref Aulib::ResamplerSrc instead.
     */
    class resampler_sdl : public resampler {
        public:
            resampler_sdl();
            ~resampler_sdl() override;

        protected:
            void do_resampling(float dst[], const float src[], std::size_t& dst_len, std::size_t& src_len) override;
            int adjust_for_output_spec(unsigned int dst_rate, unsigned int src_rate, unsigned int channels) override;
            void do_discard_pending_samples() override;

        private:
            struct impl;
            const std::unique_ptr <impl> m_pimpl;
    };
}
