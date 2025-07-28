//
// Created by igor on 3/18/25.
//

#include <iostream>
#include <musac/sdk/proc_decoder.hh>
#include <musac/sdk/samples_converter.hh>

namespace musac {
    struct proc_decoder::impl {
        explicit impl(loader_fn_t loader_fn);
        ~impl();

        loader_fn_t m_loader;
        SDL_AudioSpec m_spec;
        Uint8* m_buffer;
        Uint32 m_length;
        unsigned int m_total_samples;
        Uint8* m_ptr;
        unsigned int m_consumed;
        to_float_converter_func_t m_converter;
    };

    proc_decoder::impl::impl(loader_fn_t loader_fn)
        : m_loader(loader_fn),
          m_spec{},
          m_buffer(nullptr),
          m_length(0),
          m_total_samples(0),
          m_ptr(nullptr),
          m_consumed(0),
          m_converter(nullptr) {
    }

    proc_decoder::impl::~impl() {
        if (m_buffer) {
            SDL_free(m_buffer);
        }
    }

    proc_decoder::~proc_decoder() = default;

    bool proc_decoder::open(SDL_IOStream* rwops) {
        Uint8* src_data;
        Uint32 src_len;
        SDL_AudioSpec src_spec;
        if (m_pimpl->m_loader(rwops, false, &src_spec, &src_data, &src_len)) {
            const SDL_AudioSpec dst_spec = { SDL_AudioFormat::SDL_AUDIO_S16, 1, 44100 };
            int dst_len;
            if (!SDL_ConvertAudioSamples(&src_spec, src_data, (int)src_len, &dst_spec, &m_pimpl->m_buffer, &dst_len)) {
                SDL_free(src_data);
                return false;
            }
            m_pimpl->m_length = (unsigned int)dst_len;
            SDL_free(src_data);
            this->rewind();
            m_pimpl->m_spec = dst_spec;
            m_pimpl->m_total_samples = m_pimpl->m_length / sizeof(Uint16);
            m_pimpl->m_converter = get_to_float_conveter(m_pimpl->m_spec.format);
            return true;
        }
        return false;
    }

    unsigned int proc_decoder::get_channels() const {
        return (unsigned int)m_pimpl->m_spec.channels;
    }

    unsigned int proc_decoder::get_rate() const {
        return (unsigned int)m_pimpl->m_spec.freq;
    }

    bool proc_decoder::rewind() {
        m_pimpl->m_consumed = 0;
        return true;
    }

    std::chrono::microseconds proc_decoder::duration() const {
        return {};
    }

    bool proc_decoder::seek_to_time([[maybe_unused]] std::chrono::microseconds pos) {
        return false;
    }

    unsigned int proc_decoder::do_decode(float* const buf, unsigned  int len, [[maybe_unused]] bool& callAgain) {
        auto remains = m_pimpl->m_total_samples - m_pimpl->m_consumed;
        auto take = std::min(remains, len);
        m_pimpl->m_converter(buf, m_pimpl->m_buffer + m_pimpl->m_consumed*sizeof(uint16_t), take);
        //SDL_memcpy(buf, m_pimpl->m_buffer + m_pimpl->m_consumed*sizeof(float), len*sizeof(float));
        m_pimpl->m_consumed += take;
        return take;
    }

    proc_decoder::proc_decoder(loader_fn_t loader_fn)
        : m_pimpl(std::make_unique <impl>(loader_fn)) {
    }
}
