// This is copyrighted software. More information is at the end of this file.
#include <stdexcept>
#include <mutex>
#include <shared_mutex>
#include <thread>
#include <chrono>
#include <condition_variable>
#include <iostream>

#include <musac/stream.hh>
#include <musac/sdk/processor.hh>
#include <musac/sdk/resampler.hh>
#include <musac/sdk/buffer.hh>
#include <failsafe/failsafe.hh>
#include <musac/audio_source.hh>
#include <musac/error.hh>
#include "musac/fade_envelop.hh"
#include "musac/in_use_guard.hh"
#include "musac/audio_mixer.hh"

namespace musac {
    // Phase 2: Removed global mutex - using per-stream locks instead
    // static std::mutex s_audioMutex;  // REMOVED in Phase 2
    // Phase 3: Mixer is now thread-safe, no need for mixer mutex
    static std::atomic <bool> s_isShuttingDown{false};

    // Time tracking. Function-local so the origin is captured on first use —
    // a namespace-scope `steady_clock::now()` initializer would be dynamic
    // init, unordered relative to a client's own static constructors.
    static unsigned int get_ticks() {
        static const auto s_start_time = std::chrono::steady_clock::now();
        auto now = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast <std::chrono::milliseconds>(now - s_start_time);
        return static_cast <unsigned int>(duration.count());
    }

    // Immortal (leaked), like the global mixer: stream destructors may run from
    // a client's static teardown and still need to lock it.
    std::recursive_mutex& audio_callback_mutex() {
        static auto* m = new std::recursive_mutex();
        return *m;
    }

    // The process-wide mixer. Immortal on BOTH ends of the program's life:
    // function-local (initialized on first use, so a client calling in during
    // its own static construction never sees an uninitialized object) and
    // heap-allocated/leaked (audio_streams may be destroyed at process
    // teardown from a client's own static — e.g. a cached sound_effect — and
    // call remove_stream() from their destructor; an ordinary static could
    // already be torn down, use-after-freeing the stream container).
    static audio_mixer& global_mixer() {
        static auto* m = new audio_mixer();
        return *m;
    }

    void close_audio_stream() {
        // Prevent any further callbacks from running their per-stream logic
        s_isShuttingDown = true;
        // Then clear out the conversion stream, synchronized against a
        // callback that may be reading the device data right now
        std::lock_guard<std::recursive_mutex> cb_lock(audio_callback_mutex());
        audio_mixer::device_data().m_stream = nullptr;
    }

    void reset_audio_stream() {
        // Reset the shutdown flag to allow new devices
        s_isShuttingDown = false;
    }

    static std::atomic<int> token_generator{0};

    struct audio_stream::impl final {
        explicit impl(audio_source&& audio_src);
        ~impl();

        int m_token;

        bool m_is_open = false;
        audio_source m_audio_source;
        // Atomic: written under m_state_mutex (play/pause/resume/stop and the
        // audio callback) but read by the is_playing()/is_paused() getters,
        // which take only m_data_mutex — a different mutex, so plain bools
        // would be a C++ data race.
        std::atomic<bool> m_is_playing{false};
        std::atomic<bool> m_is_paused{false};
        float m_volume = 1.f;
        float m_stereo_pos = 0.f;
        float m_internal_volume = 1.f;
        unsigned int m_current_iteration = 0;
        unsigned int m_wanted_iterations = 0;
        unsigned int m_playback_start_tick = 0;

        enum class PendingAction { None, Pause, Stop };

        PendingAction m_pendingAction{PendingAction::None};
        fade_envelope m_fade;

        bool m_starting = false;

        std::vector <std::shared_ptr <processor>> processors;
        bool m_is_muted = false;
        callback_t m_finish_callback;
        callback_t m_loop_callback;

        std::atomic <bool> m_alive{true};
        std::atomic <uint32_t> m_inUse{0};

        // (m_lifetime_token removed — the mixer's weak_ptr<void> now
        // aliases audio_stream::m_pimpl directly, so storing a token
        // inside impl would create a self-owning cycle that prevents
        // destruction.)

        // New synchronization members for Phase 1
        mutable std::mutex m_usage_mutex;
        std::condition_variable m_usage_cv;
        std::atomic <bool> m_destruction_pending{false};

        // Phase 2: Per-stream mutexes for fine-grained locking
        mutable std::mutex m_state_mutex; // For state changes (play/pause/stop)
        mutable std::shared_mutex m_data_mutex; // For data access (volume, position, etc.)

        void stop();
        void stop_no_mixer();

        // Phase 2: Lock helper classes
        class state_lock final {
            std::lock_guard <std::mutex> m_lock;

            public:
                explicit state_lock(const impl* p)
                    : m_lock(p->m_state_mutex) {
                }
        };

        class read_lock final {
            std::shared_lock <std::shared_mutex> m_lock;

            public:
                explicit read_lock(const impl* p)
                    : m_lock(p->m_data_mutex) {
                }
        };

        class write_lock final {
            std::unique_lock <std::shared_mutex> m_lock;

            public:
                explicit write_lock(const impl* p)
                    : m_lock(p->m_data_mutex) {
                }
        };

        // Helper to create usage guard
        in_use_guard create_usage_guard() {
            bool is_valid = !m_destruction_pending.load();
            return in_use_guard(m_inUse, &m_usage_mutex, &m_usage_cv, is_valid);
        }

        void decode_audio(size_t& cur_pos, size_t len) {
            const auto device_channels = static_cast<channels_t>(
                audio_mixer::device_data().m_audio_spec.channels);
            m_audio_source.read_samples(global_mixer().m_stream_buf.data(), cur_pos, len, device_channels);
        }

        // Takes a snapshot of the processor list (copied under m_data_mutex by
        // the caller) so remove_processor()/clear_processors() on another
        // thread can't invalidate the iteration; the shared_ptr copies keep
        // the processor objects alive even if removed concurrently.
        void run_processors(const std::vector<std::shared_ptr<processor>>& procs,
                            size_t cur_pos, size_t out_offset) {
            for (const auto& proc : procs) {
                const auto len = cur_pos - out_offset;

                proc->process(global_mixer().m_processor_buf.data() + out_offset,
                              global_mixer().m_stream_buf.data() + out_offset,
                              static_cast<unsigned int>(len));
                std::memcpy(global_mixer().m_stream_buf.data() + out_offset, global_mixer().m_processor_buf.data() + out_offset,
                            len * sizeof(float));
            }
        }

        [[nodiscard]] std::tuple <float, float> eval_volume() const {
            float volume_left = m_volume * m_internal_volume;
            float volume_right = m_volume * m_internal_volume;
            const auto device_channels = audio_mixer::device_data().m_audio_spec.channels;
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
            const auto channels = static_cast<unsigned int>(audio_mixer::device_data().m_audio_spec.channels);
            const auto freq = static_cast<unsigned int>(audio_mixer::device_data().m_audio_spec.freq);
            if (!m_starting || ticks_since_play_start >= wanted_ticks) {
                return 0;
            }

            const auto out_offset_ticks = wanted_ticks - ticks_since_play_start;
            const auto offset = out_offset_ticks * channels * freq / 1000;
            return offset - (offset % channels);
        }
    };

    // ==============================================================================================================

    audio_stream::impl::impl(audio_source&& audio_src)
        : m_token(token_generator.fetch_add(1, std::memory_order_relaxed)),
          m_audio_source(std::move(audio_src)) {}

    audio_stream::impl::~impl() = default;

    void audio_stream::impl::stop() {
        stop_no_mixer();
    }

    void audio_stream::impl::stop_no_mixer() {
        m_audio_source.rewind();
        m_is_playing = false;
    }

    std::vector<float> audio_stream::get_final_output_buffer() {
        return global_mixer().get_final_output();
    }
    
    void audio_stream::audio_callback(uint8_t out[], unsigned int out_len) {
        // If we're tearing down, just emit silence and bail.
        if (s_isShuttingDown.load(std::memory_order_relaxed)) {
            std::memset(out, 0, out_len);
            return;
        }

        // Serialize the whole callback: against a second device's callback
        // (the mixer buffers and device data are process-global), against
        // stream destruction/moves (so raw audio_stream* stay valid for the
        // finish/loop callback dispatch below), and against device switching.
        std::lock_guard<std::recursive_mutex> cb_lock(audio_callback_mutex());

        auto& dev = audio_mixer::device_data();

        // Always clear the output buffer first (silence)
        std::memset(out, 0, out_len);

        // If we don't have a converter yet, we just leave the silence
        if (!dev.m_sample_converter) {
            // Log once, but don't bail out entirely—silence is safer
            static bool warned = false;
            if (!warned) {
                // LOG_WARN("audio", "missing sample converter, outputting silence");
                warned = true;
            }
            return;
        }

        const auto format = audio_mixer::device_data().m_audio_spec.format;
        const auto channels = static_cast<unsigned int>(audio_mixer::device_data().m_audio_spec.channels);
        const auto freq = static_cast<unsigned int>(audio_mixer::device_data().m_audio_spec.freq);

        // Calculate bytes per sample based on format
        unsigned int bytes_per_sample = 0;
        switch (format) {
            case audio_format::u8:
            case audio_format::s8:
                bytes_per_sample = 1;
                break;
            case audio_format::s16le:
            case audio_format::s16be:
                bytes_per_sample = 2;
                break;
            case audio_format::s32le:
            case audio_format::s32be:
            case audio_format::f32le:
            case audio_format::f32be:
                bytes_per_sample = 4;
                break;
            default:
                LOG_ERROR("audio", "Unknown audio format");
                return;
        }
        const auto out_len_samples = out_len / bytes_per_sample;
        const auto out_len_frames = out_len_samples / channels;

        global_mixer().resize(out_len_samples);
        global_mixer().set_zeros();

        // Iterate over a copy of the original stream list, since we might want to
        // modify the original as we go, removing streams that have stopped.
        auto streamList = global_mixer().get_streams();

        const auto now_tick = get_ticks();
        const auto wanted_ticks = out_len_frames * 1000 / freq;

        for (const auto& entry : *streamList) {
            // Pin the impl alive for the duration of this iteration.
            // `entry.lifetime_token` aliases the owning
            // audio_stream::m_pimpl shared_ptr, so locking it
            // promotes the weak_ptr to a strong shared_ptr<void>
            // that points at the impl. As long as `pin` is held, no
            // destructor can free the impl out from under us — which
            // closes the TOCTOU race that previously SIGSEGV'd
            // between the lifetime-token check and the m_pimpl
            // dereference when an audio_stream was destroyed (or the
            // player's event_streams_ vector reallocated) on another
            // thread during this iteration.
            //
            // We deliberately don't deref `entry.stream` (the
            // audio_stream*) — that pointer can dangle if the host
            // moved its audio_stream into a new container slot and
            // the snapshot still holds the old address. Everything
            // the callback needs is on the pinned impl itself.
            auto pin = entry.lifetime_token.lock();
            if (!pin) {
                continue; // impl has been freed
            }
            auto* stream = static_cast<impl*>(pin.get());

            in_use_guard guard = stream->create_usage_guard();
            if (!guard) {
                continue; // Stream is being destroyed, skip it
            }

            bool has_finished = false;
            bool has_looped = false;
            bool remove_from_mixer = false;

            {
                // Hold the stream's state mutex for the whole slice so API
                // methods (play/stop/pause/seek/rewind — and the destructor's
                // teardown rewind) can't mutate the source or playback state
                // mid-decode. Data-domain fields (volume, mute, processors)
                // are read under short m_data_mutex sections below; holding
                // both here means any writer, whichever mutex it takes, is
                // excluded.
                std::lock_guard<std::mutex> state_lk(stream->m_state_mutex);

                if (!stream->m_alive) {
                    continue;
                }
                if (stream->m_wanted_iterations != 0
                    && stream->m_current_iteration >= stream->m_wanted_iterations) {
                    continue;
                }
                if (stream->m_is_paused) {
                    continue;
                }

                const auto ticks_since_play_start = static_cast <int>(now_tick - stream->m_playback_start_tick);
                if (ticks_since_play_start <= 0) {
                    continue;
                }

                auto out_offset = stream->eval_out_offset(static_cast<unsigned int>(now_tick), wanted_ticks);
                size_t cur_pos = out_offset;

                stream->m_starting = false;

                std::vector<std::shared_ptr<processor>> procs;
                {
                    std::shared_lock<std::shared_mutex> data_lk(stream->m_data_mutex);
                    procs = stream->processors;
                }

                while (cur_pos < out_len_samples) {
                    stream->decode_audio(cur_pos, out_len_samples);
                    stream->run_processors(procs, cur_pos, out_offset);

                    if (cur_pos < out_len_samples) {
                        // Source didn't fill the buffer - it must have reached the end

                        // Check if we should loop
                        if (stream->m_wanted_iterations != 0) {
                            // We have a specific number of iterations requested
                            ++stream->m_current_iteration;
                            if (stream->m_current_iteration >= stream->m_wanted_iterations) {
                                stream->m_is_playing = false;
                                remove_from_mixer = true;
                                has_finished = true;
                                break;
                            }
                        }

                        // Try to rewind for the next iteration
                        bool can_rewind = stream->m_audio_source.rewind();
                        if (!can_rewind) {
                            // source is non-seekable: stop playback
                            stream->m_is_playing = false;
                            remove_from_mixer = true;
                            has_finished = true;
                            break;
                        }
                        has_looped = true;
                    }
                }

                // Apply fade envelope
                float envGain = stream->m_fade.getGain();
                float volume_left;
                float volume_right;
                bool muted;
                {
                    std::shared_lock<std::shared_mutex> data_lk(stream->m_data_mutex);
                    auto [base_left, base_right] = stream->eval_volume();
                    volume_left = base_left * envGain;
                    volume_right = base_right * envGain;
                    muted = stream->m_is_muted;
                }

                // If a fade‐out has just finished, stop and remove
                if (envGain == 0.f && stream->m_fade.getState() == fade_envelope::state::none) {
                    if (stream->m_pendingAction == impl::PendingAction::Stop) {
                        stream->m_is_playing = false;
                        stream->stop_no_mixer();
                    } else if (stream->m_pendingAction == impl::PendingAction::Pause) {
                        stream->m_is_paused = true;
                        stream->m_is_playing = false;
                    }
                    stream->m_pendingAction = impl::PendingAction::None;
                    remove_from_mixer = true;
                    has_finished = true; // only for Stop do you fire finish-callback
                }

                // Avoid mixing on zero volume.
                if (!muted && (volume_left > 0.f || volume_right > 0.f)) {
                    global_mixer().mix_channels(static_cast<channels_t>(channels),
                                               out_offset, cur_pos, volume_left, volume_right);
                }
            } // state mutex released — user callbacks below may call stream API

            // Dispatch user finish/loop callbacks. entry.stream from the
            // snapshot may be stale after a move, so look up the CURRENT
            // audio_stream* by token instead: audio_callback_mutex() (held
            // for the whole callback) blocks moves and destruction, and the
            // in_use_guard blocks a destructor that slipped in before we took
            // it — so the pointer can't go dangling while we invoke.
            // Removal happens before invocation so a finish callback that
            // calls play() re-adds the stream instead of being wiped out.
            audio_stream* current = nullptr;
            if (has_finished || has_looped) {
                current = global_mixer().find_stream_by_token(stream->m_token);
            }
            if (remove_from_mixer) {
                global_mixer().remove_stream(stream->m_token);
            }
            if (current) {
                if (has_finished) {
                    current->invoke_finish_callback();
                } else if (has_looped) {
                    current->invoke_loop_callback();
                }
            }
        } // in_use_guard released here
        
        // Check if mixer is globally muted - if so, clear the buffer
        if (global_mixer().is_all_muted()) {
            global_mixer().set_zeros();
        }
        
        // Capture the final mixed output for visualization
        global_mixer().capture_final_output(global_mixer().m_final_mix_buf.data(), out_len_samples);
        
        // Finally convert the float mix into the device buffer
        dev.m_sample_converter(out, out_len, global_mixer().m_final_mix_buf);
    }

    // =============================================================================================================="

    // ==============================================================================================================
    audio_stream::audio_stream(audio_source&& audio_src)
        : m_pimpl(std::make_shared <impl>(std::move(audio_src))) {}

    audio_stream::~audio_stream() {
        if (!m_pimpl) return;

        // Serialize against the audio callback: it holds audio_callback_mutex()
        // for its whole body, so once we own the mutex no callback is running
        // and none can start until we're done — the callback can never observe
        // a partially-destroyed stream. Also orders us against moves and
        // device switches. (If a finish/loop callback on the audio thread
        // destroys a stream, the recursive mutex lets it through.)
        std::lock_guard<std::recursive_mutex> cb_lock(audio_callback_mutex());

        // 1) Signal destruction pending to prevent new usage
        m_pimpl->m_destruction_pending.store(true);
        m_pimpl->m_alive = false;

        // 2) Remove from mixer under state lock
        {
            impl::state_lock lock(m_pimpl.get());
            global_mixer().remove_stream(m_pimpl->m_token);
            m_pimpl->stop_no_mixer();
        }

        // 3) Wait for callbacks with timeout (belt-and-suspenders: with
        //    audio_callback_mutex() held the usage count is already zero
        //    unless our own audio-thread callback is destroying us)
        {
            std::unique_lock <std::mutex> lock(m_pimpl->m_usage_mutex);
            auto wait_result = m_pimpl->m_usage_cv.wait_for(
                lock,
                std::chrono::seconds(5),
                [this] { return m_pimpl->m_inUse.load() == 0; }
            );

            if (!wait_result) {
                // Log error but continue - we've done our best
                // LOG_ERROR("audio_stream", "Timeout waiting for callbacks to complete");
            }
        }

        // 4) Clear callbacks to prevent any further calls
        {
            impl::write_lock locker(m_pimpl.get());
            m_pimpl->m_finish_callback = nullptr;
            m_pimpl->m_loop_callback = nullptr;
        }
    }

    audio_stream::audio_stream(audio_stream&& other) noexcept {
        // Take the callback mutex BEFORE hollowing out `other`: the audio
        // callback resolves streams by token under this mutex, so it must
        // never observe the window between "other.m_pimpl is null" and
        // "the container points at this".
        std::lock_guard<std::recursive_mutex> cb_lock(audio_callback_mutex());

        m_pimpl = std::move(other.m_pimpl);
        // Update the mixer to point to this new stream object
        if (m_pimpl) {
            global_mixer().update_stream_pointer(m_pimpl->m_token, this);
        }
    }

    auto audio_stream::operator=(audio_stream&& other) noexcept -> audio_stream& {
        if (this != &other) {
            // Same serialization rationale as the destructor / move ctor.
            std::lock_guard<std::recursive_mutex> cb_lock(audio_callback_mutex());

            // Clean up current stream if it exists
            if (m_pimpl) {
                // Same cleanup as destructor
                m_pimpl->m_destruction_pending.store(true);
                m_pimpl->m_alive = false; {
                    impl::state_lock lock(m_pimpl.get());
                    global_mixer().remove_stream(m_pimpl->m_token);
                    m_pimpl->stop_no_mixer();
                } {
                    std::unique_lock <std::mutex> lock(m_pimpl->m_usage_mutex);
                    m_pimpl->m_usage_cv.wait_for(
                        lock,
                        std::chrono::seconds(5),
                        [this] { return m_pimpl->m_inUse.load() == 0; }
                    );
                }

                {
                    impl::write_lock locker(m_pimpl.get());
                    m_pimpl->m_finish_callback = nullptr;
                    m_pimpl->m_loop_callback = nullptr;
                }
            }

            // Take ownership of other's implementation
            m_pimpl = std::move(other.m_pimpl);

            // Update the mixer to point to this stream object
            if (m_pimpl) {
                global_mixer().update_stream_pointer(m_pimpl->m_token, this);
            }
        }
        return *this;
    }

    void audio_stream::open() {
        // The device data read below is swapped by audio_device::create_stream
        // and device switching; the callback mutex orders us against both.
        std::lock_guard<std::recursive_mutex> cb_lock(audio_callback_mutex());
        impl::state_lock lock(m_pimpl.get());

        if (m_pimpl->m_is_open) {
            return;
        }

        auto rate = static_cast<unsigned int>(audio_mixer::device_data().m_audio_spec.freq);
        auto channels = static_cast<channels_t>(audio_mixer::device_data().m_audio_spec.channels);
        auto frame_size = static_cast<unsigned int>(audio_mixer::device_data().m_frame_size);

        m_pimpl->m_audio_source.open(rate, channels, frame_size);

        m_pimpl->m_is_open = true;
    }

    bool audio_stream::rewind() {
        try {
            open();
        } catch (const std::exception& e) {
            LOG_ERROR("audio_stream", "Failed to open stream during rewind: %s", e.what());
            return false;
        }

        impl::state_lock locker(m_pimpl.get());
        return m_pimpl->m_audio_source.rewind();
    }

    void audio_stream::set_volume(float volume) {
        impl::write_lock locker(m_pimpl.get());

        if (volume < 0.f) {
            volume = 0.f;
        }
        m_pimpl->m_volume = volume;
    }

    auto audio_stream::volume() const -> float {
        impl::read_lock locker(m_pimpl.get());

        return m_pimpl->m_volume;
    }

    void audio_stream::set_stereo_position(const float position) {
        impl::write_lock locker(m_pimpl.get());

        m_pimpl->m_stereo_pos = std::clamp(position, -1.f, 1.f);
    }

    auto audio_stream::get_stereo_position() const -> float {
        impl::read_lock locker(m_pimpl.get());

        return m_pimpl->m_stereo_pos;
    }

    void audio_stream::mute() {
        impl::write_lock locker(m_pimpl.get());

        m_pimpl->m_is_muted = true;
    }

    void audio_stream::unmute() {
        impl::write_lock locker(m_pimpl.get());

        m_pimpl->m_is_muted = false;
    }

    auto audio_stream::is_muted() const -> bool {
        impl::read_lock locker(m_pimpl.get());

        return m_pimpl->m_is_muted;
    }

    auto audio_stream::is_playing() const -> bool {
        impl::read_lock locker(m_pimpl.get());

        return m_pimpl->m_is_playing;
    }

    auto audio_stream::is_paused() const -> bool {
        impl::read_lock locker(m_pimpl.get());

        return m_pimpl->m_is_paused;
    }

    auto audio_stream::duration() const -> std::chrono::microseconds {
        // state_lock, not read_lock: the audio source is state-domain (the
        // callback decodes and seek_to_time() seeks under m_state_mutex)
        impl::state_lock locker(m_pimpl.get());

        return m_pimpl->m_audio_source.duration();
    }

    auto audio_stream::seek_to_time(std::chrono::microseconds pos) -> bool {
        impl::state_lock locker(m_pimpl.get());
        return m_pimpl->m_audio_source.seek_to_time(pos);
    }

    void audio_stream::set_finish_callback(callback_t func) const {
        impl::write_lock locker(m_pimpl.get());
        m_pimpl->m_finish_callback = std::move(func);
    }

    void audio_stream::remove_finish_callback() const {
        impl::write_lock locker(m_pimpl.get());
        m_pimpl->m_finish_callback = nullptr;
    }

    void audio_stream::set_loop_callback(callback_t func) const {
        impl::write_lock locker(m_pimpl.get());
        m_pimpl->m_loop_callback = std::move(func);
    }

    void audio_stream::remove_loop_callback() const {
        impl::write_lock locker(m_pimpl.get());
        m_pimpl->m_loop_callback = nullptr;
    }

    void audio_stream::add_processor(std::shared_ptr <processor> processor_obj) const {
        impl::write_lock locker(m_pimpl.get());

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
        impl::write_lock locker(m_pimpl.get());

        auto it =
            std::find_if(m_pimpl->processors.begin(), m_pimpl->processors.end(),
                         [&processor_obj](std::shared_ptr <processor>& p) { return p.get() == processor_obj; });
        if (it == m_pimpl->processors.end()) {
            return;
        }
        m_pimpl->processors.erase(it);
    }

    void audio_stream::clear_processors() {
        impl::write_lock locker(m_pimpl.get());

        m_pimpl->processors.clear();
    }

    void audio_stream::invoke_finish_callback() {
        // Copy under the data lock, invoke without it: the callback may call
        // set_finish_callback()/remove_finish_callback() itself.
        callback_t cb;
        {
            impl::read_lock locker(m_pimpl.get());
            cb = m_pimpl->m_finish_callback;
        }
        if (cb) {
            // Call directly from audio thread - callbacks should be quick!
            cb(*this);
        }
    }

    void audio_stream::invoke_loop_callback() {
        callback_t cb;
        {
            impl::read_lock locker(m_pimpl.get());
            cb = m_pimpl->m_loop_callback;
        }
        if (cb) {
            // Call directly from audio thread - callbacks should be quick!
            cb(*this);
        }
    }

    void audio_stream::set_audio_device_data(const audio_device_data& aud) {
        // The audio callback and audio_stream::open() read this global under
        // the same mutex.
        std::lock_guard<std::recursive_mutex> cb_lock(audio_callback_mutex());
        audio_mixer::device_data() = aud;
    }

    int audio_stream::get_token() const {
        return m_pimpl->m_token;
    }

    audio_mixer& audio_stream::get_global_mixer() {
        return global_mixer();
    }

    audio_stream::stream_snapshot audio_stream::capture_state() const {
        // Hold both stream mutexes: the fields below span the state domain
        // (playback flags, fade) and the data domain (volume, stereo, mute).
        impl::state_lock state_locker(m_pimpl.get());
        impl::read_lock data_locker(m_pimpl.get());

        stream_snapshot snapshot;
        snapshot.playback_tick = m_pimpl->m_playback_start_tick;
        snapshot.volume = m_pimpl->m_volume;
        snapshot.internal_volume = m_pimpl->m_internal_volume;
        snapshot.stereo_pos = m_pimpl->m_stereo_pos;
        snapshot.is_playing = m_pimpl->m_is_playing;
        snapshot.is_paused = m_pimpl->m_is_paused;
        snapshot.is_muted = m_pimpl->m_is_muted;
        snapshot.starting = m_pimpl->m_starting;
        snapshot.current_iteration = m_pimpl->m_current_iteration;
        snapshot.wanted_iterations = m_pimpl->m_wanted_iterations;
        snapshot.fade_gain = m_pimpl->m_fade.getGain();
        snapshot.fade_state = static_cast <int>(m_pimpl->m_fade.getState());
        snapshot.playback_start_tick = m_pimpl->m_playback_start_tick;
        return snapshot;
    }

    void audio_stream::restore_state(const stream_snapshot& state) {
        impl::state_lock state_locker(m_pimpl.get());
        impl::write_lock data_locker(m_pimpl.get());

        m_pimpl->m_playback_start_tick =
            static_cast<unsigned int>(state.playback_start_tick);
        m_pimpl->m_volume = state.volume;
        m_pimpl->m_internal_volume = state.internal_volume;
        m_pimpl->m_stereo_pos = state.stereo_pos;
        m_pimpl->m_is_playing = state.is_playing;
        m_pimpl->m_is_paused = state.is_paused;
        m_pimpl->m_is_muted = state.is_muted;
        m_pimpl->m_starting = state.starting;
        m_pimpl->m_current_iteration =
            static_cast<unsigned int>(state.current_iteration);
        m_pimpl->m_wanted_iterations =
            static_cast<unsigned int>(state.wanted_iterations);
        // TODO: Restore fade state properly
    }

    bool audio_stream::play(unsigned int iterations, std::chrono::microseconds fadeTime) {
        try {
            open();
        } catch (const std::exception& e) {
            LOG_ERROR("audio_stream", "Failed to open stream during play: %s", e.what());
            return false;
        }

        impl::state_lock locker(m_pimpl.get());
        if (m_pimpl->m_is_playing) {
            return true;
        }

        // Initialize playback
        m_pimpl->m_current_iteration = 0;
        m_pimpl->m_wanted_iterations = iterations;
        m_pimpl->m_playback_start_tick = get_ticks();
        m_pimpl->m_starting = true;

        // Fade-in if requested
        if (fadeTime.count() > 0) {
            m_pimpl->m_fade.startFadeIn(
                std::chrono::duration_cast <std::chrono::milliseconds>(fadeTime)
            );
        }

        m_pimpl->m_is_playing = true;
        global_mixer().add_stream(this, std::weak_ptr<void>(std::shared_ptr<void>(m_pimpl)));
        return true;
    }

    void audio_stream::stop(std::chrono::microseconds fadeTime) {
        impl::state_lock locker(m_pimpl.get());
        if (fadeTime.count() > 0) {
            m_pimpl->m_pendingAction = impl::PendingAction::Stop;
            // Fade-out and defer stop until envelope completes
            m_pimpl->m_fade.startFadeOut(
                std::chrono::duration_cast <std::chrono::milliseconds>(fadeTime)
            );
        } else {
            // Immediate stop
            global_mixer().remove_stream(m_pimpl->m_token);
            m_pimpl->stop_no_mixer();
        }
    }

    void audio_stream::pause(std::chrono::microseconds fadeTime) {
        try {
            open();
        } catch (const std::exception& e) {
            LOG_ERROR("audio_stream", "Failed to open stream during pause: %s", e.what());
            return;
        }
        impl::state_lock locker(m_pimpl.get());
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
            m_pimpl->m_is_playing = false;
        }
    }

    void audio_stream::resume(std::chrono::microseconds fadeTime) {
        impl::state_lock locker(m_pimpl.get());

        // 1) If we're already playing (and no fade is active), do nothing
        if (m_pimpl->m_is_playing &&
            m_pimpl->m_fade.getState() == fade_envelope::state::none &&
            !m_pimpl->m_is_paused) {
            return;
        }

        // 2) Cancel any pending pause/stop
        m_pimpl->m_pendingAction = impl::PendingAction::None;

        // 3) Mark as not paused
        bool was_paused = m_pimpl->m_is_paused;
        m_pimpl->m_is_paused = false;
        m_pimpl->m_is_playing = true;

        // 4) Only add to mixer if we were actually paused (not already in mixer)
        if (was_paused) {
            global_mixer().add_stream(this, std::weak_ptr<void>(std::shared_ptr<void>(m_pimpl)));
        }

        // 4) Always restart fade-in if requested (this must reset any fade-out)
        if (fadeTime.count() > 0) {
            m_pimpl->m_fade.startFadeIn(
                std::chrono::duration_cast <std::chrono::milliseconds>(fadeTime)
            );
        } else {
            // Jump straight to full volume
            m_pimpl->m_internal_volume = 1.0f;
            m_pimpl->m_fade.reset(); // make sure envelope is idle
        }
    }
}
