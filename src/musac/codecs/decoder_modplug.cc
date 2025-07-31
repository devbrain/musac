// This is copyrighted software. More information is at the end of this file.
#include <musac/codecs/decoder_modplug.hh>

#include <musac/sdk/buffer.hh>
#include "musac/codecs/libmodplug/modplug.h"
#include <limits>

namespace chrono = std::chrono;

namespace musac {
    struct decoder_modplug::impl final {
        impl();

        std::unique_ptr <ModPlugFile, decltype(&ModPlug_Unload)> handle{nullptr, &ModPlug_Unload};
        bool m_eof = false;
        chrono::microseconds m_duration{};
        ModPlug_Settings settings;
        static bool initialized;
    };

    bool decoder_modplug::impl::initialized = false;

    decoder_modplug::impl::impl()
        : settings{} {
        if (!initialized) {
            ModPlug_Init();
            initialized = true;
        }
        SDL_zero(settings);

        /* The settings will require some experimenting. I've borrowed some
            of them from the XMMS ModPlug plugin. */
        settings.mFlags = MODPLUG_ENABLE_OVERSAMPLING;
        settings.mFlags |= MODPLUG_ENABLE_NOISE_REDUCTION |
            MODPLUG_ENABLE_MEGABASS |
            MODPLUG_ENABLE_SURROUND;

        settings.mReverbDepth = 30;
        settings.mReverbDelay = 100;
        settings.mBassAmount = 40;
        settings.mBassRange = 30;
        settings.mSurroundDepth = 20;
        settings.mSurroundDelay = 20;
        settings.mChannels = 2;
        settings.mBits = SDL_AUDIO_BITSIZE(SDL_AudioFormat::SDL_AUDIO_S32);
        settings.mFrequency = 44100;
        settings.mResamplingMode = MODPLUG_RESAMPLE_FIR;
        settings.mLoopCount = 0;
    }

    decoder_modplug::decoder_modplug()
        : m_pimpl(std::make_unique <impl>()) {
    }

    decoder_modplug::~decoder_modplug() = default;

    bool decoder_modplug::open(SDL_IOStream* rwops) {
        if (is_open()) {
            return true;
        }
        // FIXME: error reporting
        Sint64 dataSize = SDL_GetIOSize(rwops);
        if (dataSize <= 0 || dataSize > std::numeric_limits <int>::max()) {
            return false;
        }
        buffer <Uint8> data(static_cast <unsigned >(dataSize));
        if (SDL_ReadIO(rwops, data.data(), data.size()) != static_cast<size_t>(dataSize)) {
            return false;
        }
        m_pimpl->handle.reset(ModPlug_Load(data.data(), (int)data.size(), &m_pimpl->settings));
        if (!m_pimpl->handle) {
            return false;
        }
        //ModPlug_SetMasterVolume(m_pimpl->mpHandle.get(), 192);
        m_pimpl->m_duration = chrono::milliseconds(ModPlug_GetLength(m_pimpl->handle.get()));
        set_is_open(true);
        return true;
    }

    unsigned int decoder_modplug::get_channels() const {
        return (unsigned int)m_pimpl->settings.mChannels;
    }

    unsigned int decoder_modplug::get_rate() const {
        return (unsigned int)m_pimpl->settings.mFrequency;
    }

    unsigned int decoder_modplug::do_decode(float buf[], unsigned int len, bool& /*callAgain*/) {
        if (m_pimpl->m_eof || !is_open()) {
            return 0;
        }
        buffer <Sint32> tmpBuf(len);
        int ret = ModPlug_Read(m_pimpl->handle.get(), tmpBuf.data(), (int)len * 4);
        // Convert from 32-bit to float.
        for (unsigned int i = 0; i < len; ++i) {
            buf[i] = (float)tmpBuf[i] / 2147483648.f;
        }
        if (ret == 0) {
            m_pimpl->m_eof = true;
        }
        return (unsigned int)ret / sizeof(*buf);
    }

    bool decoder_modplug::rewind() {
        return seek_to_time(chrono::microseconds::zero());
    }

    chrono::microseconds decoder_modplug::duration() const {
        return m_pimpl->m_duration;
    }

    bool decoder_modplug::seek_to_time(chrono::microseconds pos) {
        if (!is_open()) {
            return false;
        }
        ModPlug_Seek(m_pimpl->handle.get(), (int)chrono::duration_cast <chrono::milliseconds>(pos).count());
        m_pimpl->m_eof = false;
        return true;
    }
}
