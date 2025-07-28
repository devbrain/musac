// This is copyrighted software. More information is at the end of this file.
#include "resampler_sdl.hh"
#include <SDL3/SDL.h>

namespace musac {
    struct resampler_sdl::impl final {
        std::unique_ptr <SDL_AudioStream, decltype(&SDL_DestroyAudioStream)> m_resampler{
            nullptr, &SDL_DestroyAudioStream
        };
    };

    resampler_sdl::resampler_sdl()
        : m_pimpl(std::make_unique <impl>()) {
    }

    resampler_sdl::~resampler_sdl() = default;

    void resampler_sdl::do_resampling(float dst[], const float src[], std::size_t& dst_len, std::size_t& src_len) {
        if (!m_pimpl->m_resampler) {
            dst_len = src_len = 0;
            return;
        }

        if (!SDL_PutAudioStreamData(m_pimpl->m_resampler.get(), src, static_cast <int>(src_len * sizeof(float)))) {
            dst_len = src_len = 0;
            return;
        }

        const auto bytes_resampled = SDL_GetAudioStreamData(m_pimpl->m_resampler.get(), dst, (int)(dst_len * sizeof(float)));
        if (bytes_resampled < 0) {
            dst_len = src_len = 0;
            return;
        }
        dst_len = static_cast <unsigned int>(bytes_resampled) / sizeof(float);
    }

    int resampler_sdl::adjust_for_output_spec(unsigned int dst_rate, unsigned int src_rate,
                                              unsigned int channels) {
        const SDL_AudioSpec src_spec{SDL_AUDIO_F32, (int)channels, (int)src_rate};
        const SDL_AudioSpec dst_spec{SDL_AUDIO_F32, (int)channels, (int)dst_rate};
        m_pimpl->m_resampler.reset(SDL_CreateAudioStream(&src_spec, &dst_spec));
        if (!m_pimpl->m_resampler) {
            return -1;
        }
        return 0;
    }

    void resampler_sdl::do_discard_pending_samples() {
        if (m_pimpl->m_resampler) {
            SDL_ClearAudioStream(m_pimpl->m_resampler.get());
        }
    }
}
