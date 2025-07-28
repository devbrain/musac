// This is copyrighted software. More information is at the end of this file.
#include <stdexcept>
#include <mutex>
#include <thread>

#include <musac/stream.hh>
#include <musac/sdk/processor.hh>
#include <musac/sdk/resampler.hh>
#include <musac/sdk/buffer.hh>
#include "mutex.hh"
#include <musac/audio_source.hh>
#include "fade_envelop.hh"
#include "in_use_guard.hh"
#include "callback_dispatcher.hh"
#include "audio_mixer.hh"

namespace musac {
    // A single mutex that protects all audio_stream public methods
    static std::mutex s_audioMutex;
    static std::atomic <bool> s_isShuttingDown{false};



    void close_audio_stream() {
        // Prevent any further callbacks from running their per-stream logic
        s_isShuttingDown = true;
        // Then clear out the conversion stream
        audio_mixer::m_audio_device_data.m_stream = nullptr;
    }

    static int token_generator = 0;

    struct audio_stream::impl final {
        explicit impl(audio_source&& audio_src);
        ~impl();

        int m_token;

        bool m_is_open = false;
        audio_source m_audio_source;
        bool m_is_playing = false;
        bool m_is_paused = false;
        float m_volume = 1.f;
        float m_stereo_pos = 0.f;
        float m_internal_volume = 1.f;
        unsigned int m_current_iteration = 0;
        unsigned int m_wanted_iterations = 0;
        unsigned int m_playback_start_tick = 0;

        enum class PendingAction { None, Pause, Stop };

        PendingAction m_pendingAction{PendingAction::None};
        FadeEnvelope m_fade;

        bool m_starting = false;

        std::vector <std::shared_ptr <processor>> processors;
        bool m_is_muted = false;
        callback_t m_finish_callback;
        callback_t m_loop_callback;

        std::atomic <bool> m_alive{true};
        std::atomic <uint32_t> m_inUse{0};

        static audio_mixer s_mixer;

        void stop();
        void stop_no_mixer();

        void decode_audio(unsigned int& cur_pos, unsigned int len) {
            const auto device_channels = (unsigned int)audio_mixer::m_audio_device_data.m_audio_spec.channels;
            m_audio_source.read_samples(s_mixer.m_stream_buf.data(), cur_pos, len, device_channels);
        }

        void run_processors(unsigned int cur_pos, unsigned int out_offset) {
            for (const auto& proc : processors) {
                const auto len = cur_pos - out_offset;

                proc->process(s_mixer.m_processor_buf.data() + out_offset, s_mixer.m_stream_buf.data() + out_offset,
                              len);
                std::memcpy(s_mixer.m_stream_buf.data() + out_offset, s_mixer.m_processor_buf.data() + out_offset,
                            len * sizeof(float));
            }
        }

        [[nodiscard]] std::tuple <float, float> eval_volume() const {
            float volume_left = m_volume * m_internal_volume;
            float volume_right = m_volume * m_internal_volume;
            const auto device_channels = audio_mixer::m_audio_device_data.m_audio_spec.channels;
            if (device_channels > 1) {
                if (m_stereo_pos < 0.f) {
                    volume_right *= 1.f + m_stereo_pos;
                } else if (m_stereo_pos > 0.f) {
                    volume_left *= 1.f - m_stereo_pos;
                }
            }
            return {volume_left, volume_right};
        }

        [[nodiscard]] unsigned int eval_out_offset(unsigned int now_tick, unsigned int wanted_ticks) const {
            const auto ticks_since_play_start = now_tick - m_playback_start_tick;
            const auto channels = (unsigned int)audio_mixer::m_audio_device_data.m_audio_spec.channels;
            const auto freq = (unsigned int)audio_mixer::m_audio_device_data.m_audio_spec.freq;
            if (!m_starting || ticks_since_play_start >= wanted_ticks) {
                return 0;
            }

            const auto out_offset_ticks = wanted_ticks - ticks_since_play_start;
            const auto offset = out_offset_ticks * channels * freq / 1000;
            return offset - (offset % channels);
        }
    };

    audio_mixer audio_stream::impl::s_mixer;

    // ==============================================================================================================

    audio_stream::impl::impl(audio_source&& audio_src)
        : m_token(token_generator++),
          m_audio_source(std::move(audio_src)) {
    }

    audio_stream::impl::~impl() = default;

    void audio_stream::impl::stop() {
        stop_no_mixer();
    }

    void audio_stream::impl::stop_no_mixer() {
        m_audio_source.rewind();
        m_is_playing = false;
    }

    void audio_stream::sdl_audio_callback(Uint8 out[], unsigned int out_len) {
        // If we’re tearing down, just emit silence and bail.
        if (s_isShuttingDown.load(std::memory_order_relaxed)) {
            std::memset(out, 0, out_len);
            return;
        }

        auto& dev = audio_mixer::m_audio_device_data;

        // Always clear the output buffer first (silence)
        std::memset(out, 0, out_len);

        // If we don’t have a stream or converter yet, we just leave the silence
        if (!dev.m_stream || !dev.m_sample_converter) {
            // Log once, but don’t bail out entirely—silence is safer
            static bool warned = false;
            if (!warned) {
                SDL_LogWarn(SDL_LOG_CATEGORY_AUDIO,
                            "audio callback: missing stream or converter, outputting silence");
                warned = true;
            }
            return;
        }

        const auto format = audio_mixer::m_audio_device_data.m_audio_spec.format;
        const auto channels = (unsigned int)audio_mixer::m_audio_device_data.m_audio_spec.channels;
        const auto freq = (unsigned int)audio_mixer::m_audio_device_data.m_audio_spec.freq;

        const auto out_len_samples = out_len / (SDL_AUDIO_BITSIZE((unsigned)format) / 8);
        const auto out_len_frames = out_len_samples / channels;

        impl::s_mixer.resize(out_len_samples);
        impl::s_mixer.set_zeros();

        // Iterate over a copy of the original stream list, since we might want to
        // modify the original as we go, removing streams that have stopped.
        auto streamList = impl::s_mixer.get_streams();

        const auto now_tick = SDL_GetTicks();
        const auto wanted_ticks = out_len_frames * 1000 / freq;

        for (auto* stream : *streamList) {
            InUseGuard guard(stream->m_pimpl->m_inUse);

            if (!stream->m_pimpl->m_alive) {
                continue;
            }
            if (stream->m_pimpl->m_wanted_iterations != 0
                && stream->m_pimpl->m_current_iteration >= stream->m_pimpl->m_wanted_iterations) {
                continue;
            }
            if (stream->m_pimpl->m_is_paused) {
                continue;
            }

            const auto ticks_since_play_start = static_cast <int>(now_tick - stream->m_pimpl->m_playback_start_tick);
            if (ticks_since_play_start <= 0) {
                continue;
            }

            bool has_finished = false;
            bool has_looped = false;
            auto out_offset = stream->m_pimpl->eval_out_offset((unsigned int)now_tick, wanted_ticks);
            auto cur_pos = out_offset;

            stream->m_pimpl->m_starting = false;

            while (cur_pos < out_len_samples) {
                stream->m_pimpl->decode_audio(cur_pos, out_len_samples);
                stream->m_pimpl->run_processors(cur_pos, out_offset);

                if (cur_pos < out_len_samples) {
                    // Attempt to rewind; if unsupported, leave remainder silent
                    bool can_rewind = stream->m_pimpl->m_audio_source.rewind();
                    if (!can_rewind) {
                        // source is non-seekable: break to leave silence
                        break;
                    }
                    // Only count loops if we actually rewound
                    if (stream->m_pimpl->m_wanted_iterations != 0) {
                        ++stream->m_pimpl->m_current_iteration;
                        if (stream->m_pimpl->m_current_iteration >= stream->m_pimpl->m_wanted_iterations) {
                            stream->m_pimpl->m_is_playing = false;
                            impl::s_mixer.remove_stream(stream->m_pimpl->m_token);
                            has_finished = true;
                            break;
                        }
                        has_looped = true;
                    }
                }
            }

            // Apply fade envelope
            float envGain = stream->m_pimpl->m_fade.getGain();
            auto [base_left, base_right] = stream->m_pimpl->eval_volume();
            float volume_left = base_left * envGain;
            float volume_right = base_right * envGain;

            // If a fade‐out has just finished, stop and remove
            if (envGain == 0.f && stream->m_pimpl->m_fade.getState() == FadeEnvelope::State::None) {
                if (stream->m_pimpl->m_pendingAction == impl::PendingAction::Stop) {
                    stream->m_pimpl->m_is_playing = false;
                } else if (stream->m_pimpl->m_pendingAction == impl::PendingAction::Pause) {
                    stream->m_pimpl->m_is_paused = true;
                }
                impl::s_mixer.remove_stream(stream->m_pimpl->m_token);
                stream->m_pimpl->m_pendingAction = impl::PendingAction::None;
                has_finished = true; // only for Stop do you fire finish-callback
            }

            if (stream->m_pimpl->m_fade.getState() == FadeEnvelope::State::None
                && envGain == 0.f) {
                // Completed fade‐out
                stream->m_pimpl->m_is_playing = false;
                impl::s_mixer.remove_stream(stream->m_pimpl->m_token);
                has_finished = true;
            }

            // Avoid mixing on zero volume.
            if (!stream->m_pimpl->m_is_muted && (volume_left > 0.f || volume_right > 0.f)) {
                impl::s_mixer.mix_channels(channels, out_offset, cur_pos, volume_left, volume_right);
            }

            if (has_finished) {
                stream->invoke_finish_callback();
            } else if (has_looped) {
                stream->invoke_loop_callback();
            }
        }
        // Finally convert the float mix into the device buffer
        dev.m_sample_converter(out, out_len, impl::s_mixer.m_final_mix_buf);
    }

    // ==============================================================================================================
    class sdl_audio_locker final {
        public:
            sdl_audio_locker() { s_audioMutex.lock(); }
            ~sdl_audio_locker() { s_audioMutex.unlock(); }
            sdl_audio_locker(const sdl_audio_locker&) = delete;
            auto operator=(const sdl_audio_locker&) -> sdl_audio_locker& = delete;
    };

    // ==============================================================================================================
    audio_stream::audio_stream(audio_source&& audio_src)
        : m_pimpl(std::make_unique <impl>(std::move(audio_src))) {
    }

    audio_stream::~audio_stream() {
        if (!m_pimpl) return;
        m_pimpl->m_alive = false;
        // 1) Tell the mixer to stop including us (acquires m_streams_mtx)
        impl::s_mixer.remove_stream(m_pimpl->m_token);
        // wait for any in-flight callbacks to finish
        while (m_pimpl->m_inUse.load(std::memory_order_acquire) != 0) {
            std::this_thread::yield();
        }
        // 2) Lock the audio stream and rewind/stop it
        {
            sdl_audio_locker lock;
            // stop_no_mixer(): just rewinds and flips m_is_playing = false
            m_pimpl->stop_no_mixer();
        }

        // 3) Prevent any further callbacks from being enqueued
        m_pimpl->m_finish_callback = nullptr;
        m_pimpl->m_loop_callback = nullptr;

        // 4) Purge any pending in-process callbacks for *this*
        CallbackDispatcher::instance().cleanup(m_pimpl->m_token);
    }

    audio_stream::audio_stream(audio_stream&& other) noexcept
        : m_pimpl(std::move(other.m_pimpl)) {
    }

    auto audio_stream::open() -> bool {
        sdl_audio_locker lock;

        if (m_pimpl->m_is_open) {
            return true;
        }

        auto rate = (unsigned int)audio_mixer::m_audio_device_data.m_audio_spec.freq;
        auto channels = (unsigned int)audio_mixer::m_audio_device_data.m_audio_spec.channels;
        auto frame_size = (unsigned int)audio_mixer::m_audio_device_data.m_frame_size;

        if (m_pimpl->m_audio_source.open(rate, channels, frame_size)) {
            m_pimpl->m_is_open = true;
            return true;
        }
        return false;
    }

    auto audio_stream::rewind() -> bool {
        if (!open()) {
            return false;
        }

        sdl_audio_locker locker;
        return m_pimpl->m_audio_source.rewind();
    }

    void audio_stream::set_volume(float volume) {
        sdl_audio_locker locker;

        if (volume < 0.f) {
            volume = 0.f;
        }
        m_pimpl->m_volume = volume;
    }

    auto audio_stream::volume() const -> float {
        sdl_audio_locker locker;

        return m_pimpl->m_volume;
    }

    void audio_stream::set_stereo_position(const float position) {
        sdl_audio_locker locker;

        m_pimpl->m_stereo_pos = std::clamp(position, -1.f, 1.f);
    }

    auto audio_stream::get_stereo_position() const -> float {
        sdl_audio_locker locker;

        return m_pimpl->m_stereo_pos;
    }

    void audio_stream::mute() {
        sdl_audio_locker locker;

        m_pimpl->m_is_muted = true;
    }

    void audio_stream::unmute() {
        sdl_audio_locker locker;

        m_pimpl->m_is_muted = false;
    }

    auto audio_stream::is_muted() const -> bool {
        sdl_audio_locker locker;

        return m_pimpl->m_is_muted;
    }

    auto audio_stream::is_playing() const -> bool {
        sdl_audio_locker locker;

        return m_pimpl->m_is_playing;
    }

    auto audio_stream::is_paused() const -> bool {
        sdl_audio_locker locker;

        return m_pimpl->m_is_paused;
    }

    auto audio_stream::duration() const -> std::chrono::microseconds {
        sdl_audio_locker locker;

        return m_pimpl->m_audio_source.duration();
    }

    auto audio_stream::seek_to_time(std::chrono::microseconds pos) -> bool {
        sdl_audio_locker locker;
        return m_pimpl->m_audio_source.seek_to_time(pos);
    }

    void audio_stream::set_finish_callback(callback_t func) const {
        sdl_audio_locker locker;
        m_pimpl->m_finish_callback = std::move(func);
    }

    void audio_stream::remove_finish_callback() const {
        sdl_audio_locker locker;
        m_pimpl->m_finish_callback = nullptr;
    }

    void audio_stream::set_loop_callback(callback_t func) const {
        sdl_audio_locker locker;
        m_pimpl->m_loop_callback = std::move(func);
    }

    void audio_stream::remove_loop_callback() const {
        sdl_audio_locker locker;
        m_pimpl->m_loop_callback = nullptr;
    }

    void audio_stream::add_processor(std::shared_ptr <processor> processor_obj) const {
        sdl_audio_locker locker;

        if (!processor_obj
            || std::find_if(
                m_pimpl->processors.begin(), m_pimpl->processors.end(),
                [&processor_obj](std::shared_ptr <processor>& p) { return p.get() == processor_obj.get(); })
            != m_pimpl->processors.end()) {
            return;
        }
        m_pimpl->processors.push_back(std::move(processor_obj));
    }

    void audio_stream::remove_processor(processor* processor_obj) {
        sdl_audio_locker locker;

        auto it =
            std::find_if(m_pimpl->processors.begin(), m_pimpl->processors.end(),
                         [&processor_obj](std::shared_ptr <processor>& p) { return p.get() == processor_obj; });
        if (it == m_pimpl->processors.end()) {
            return;
        }
        m_pimpl->processors.erase(it);
    }

    void audio_stream::clear_processors() {
        sdl_audio_locker locker;

        m_pimpl->processors.clear();
    }

    void audio_stream::HandleEvent([[maybe_unused]] const SDL_Event& ev) {
        CallbackDispatcher::instance().dispatch();
    }

    void audio_stream::invoke_finish_callback() {
        if (m_pimpl->m_finish_callback) {
            CallbackDispatcher::instance().enqueue({m_pimpl->m_token, [this]() {
                if (m_pimpl->m_finish_callback)
                    m_pimpl->m_finish_callback(*this);
            }});
        }
    }

    void audio_stream::invoke_loop_callback() {
        if (m_pimpl->m_loop_callback) {
            CallbackDispatcher::instance().enqueue({m_pimpl->m_token, [this]() {
                if (m_pimpl->m_loop_callback)
                    m_pimpl->m_loop_callback(*this);
            }});
        }
    }

    void audio_stream::set_audio_device_data(const audio_device_data& aud) {
        audio_mixer::m_audio_device_data = aud;
    }

    int audio_stream::get_token() const {
        return m_pimpl->m_token;
    }



    bool audio_stream::play(unsigned int iterations, std::chrono::microseconds fadeTime) {
        if (!open()) {
            return false;
        }

        sdl_audio_locker locker;
        if (m_pimpl->m_is_playing) {
            return true;
        }

        // Initialize playback
        m_pimpl->m_current_iteration = 0;
        m_pimpl->m_wanted_iterations = iterations;
        m_pimpl->m_playback_start_tick = static_cast <unsigned int>(SDL_GetTicks());
        m_pimpl->m_starting = true;

        // Fade-in if requested
        if (fadeTime.count() > 0) {
            m_pimpl->m_fade.startFadeIn(
                std::chrono::duration_cast <std::chrono::milliseconds>(fadeTime)
            );
        }

        m_pimpl->m_is_playing = true;
        impl::s_mixer.add_stream(this);
        return true;
    }

    void audio_stream::stop(std::chrono::microseconds fadeTime) {
        sdl_audio_locker locker;
        if (fadeTime.count() > 0) {
            m_pimpl->m_pendingAction = impl::PendingAction::Stop;
            // Fade-out and defer stop until envelope completes
            m_pimpl->m_fade.startFadeOut(
                std::chrono::duration_cast <std::chrono::milliseconds>(fadeTime)
            );
        } else {
            // Immediate stop
            impl::s_mixer.remove_stream(m_pimpl->m_token);
            m_pimpl->stop_no_mixer();
        }
    }

    void audio_stream::pause(std::chrono::microseconds fadeTime) {
        if (!open()) return;
        sdl_audio_locker locker;
        if (m_pimpl->m_is_paused) return;

        if (fadeTime.count() > 0) {
            // Fade-out then pause
            m_pimpl->m_pendingAction = impl::PendingAction::Pause;
            m_pimpl->m_fade.startFadeOut(
                std::chrono::duration_cast <std::chrono::milliseconds>(fadeTime)
            );
            // Actual pause occurs when envelope finishes
        } else {
            m_pimpl->m_is_paused = true;
        }
    }

    void audio_stream::resume(std::chrono::microseconds fadeTime) {
        sdl_audio_locker locker;

        // 1) If we're already playing (and no fade is active), do nothing
        if (m_pimpl->m_is_playing &&
            m_pimpl->m_fade.getState() == FadeEnvelope::State::None &&
            !m_pimpl->m_is_paused)
        {
            return;
        }

        // 2) Cancel any pending pause/stop
        m_pimpl->m_pendingAction = impl::PendingAction::None;

        // 3) Mark as not paused, and re-add to mixer
        m_pimpl->m_is_paused  = false;
        m_pimpl->m_is_playing = true;
        impl::s_mixer.add_stream(this);

        // 4) Always restart fade-in if requested (this must reset any fade-out)
        if (fadeTime.count() > 0) {
            m_pimpl->m_fade.startFadeIn(
                std::chrono::duration_cast<std::chrono::milliseconds>(fadeTime)
            );
        } else {
            // Jump straight to full volume
            m_pimpl->m_internal_volume = 1.0f;
            m_pimpl->m_fade.reset();        // make sure envelope is idle
        }
    }
}
