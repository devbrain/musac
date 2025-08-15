/**
 * @file stream.hh
 * @brief Audio stream playback control
 * @ingroup streams
 */

// This is copyrighted software. More information is at the end of this file.
#pragma once

#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <musac/sdk/buffer.hh>
#include <musac/audio_source.hh>
#include <musac/audio_device_data.hh>
#include <musac/export_musac.h>
#include <musac/sdk/from_float_converter.hh>

namespace musac {
    class decoder;
    class resampler;
    class processor;
    class audio_device;
    class audio_mixer;

    /**
     * @class audio_stream
     * @brief Manages playback of audio from various sources
     * @ingroup streams
     * 
     * The audio_stream class handles audio playback control including play, pause,
     * stop, volume, stereo positioning, and fade effects. Streams are created from
     * audio_device objects and play audio_source data.
     * 
     * ## Basic Usage
     * 
     * @code
     * // Load and play audio
     * auto source = audio_loader::load("music.mp3");
     * auto stream = device.create_stream(std::move(source));
     * stream.play();
     * 
     * // Control playback
     * stream.set_volume(0.5f);
     * stream.pause();
     * stream.resume();
     * stream.stop();
     * @endcode
     * 
     * ## Looping
     * 
     * @code
     * stream.play(3);  // Play 3 times
     * stream.play(0);  // Loop forever
     * 
     * // With loop callback
     * stream.set_loop_callback([](audio_stream& s) {
     *     std::cout << "Loop!" << std::endl;
     * });
     * @endcode
     * 
     * ## Fade Effects
     * 
     * @code
     * using namespace std::chrono_literals;
     * 
     * // Fade in over 2 seconds
     * stream.play(1, 2s);
     * 
     * // Fade out and stop after 1 second
     * stream.stop(1s);
     * @endcode
     * 
     * ## State Machine
     * 
     * The stream has three basic states:
     * - **Stopped**: Initial state, not playing
     * - **Playing**: Actively playing audio
     * - **Paused**: Temporarily suspended
     * 
     * During fade operations, the stream remains in its current state until
     * the fade completes. For example, stop() with fade keeps is_playing() 
     * returning true until the fade finishes.
     * 
     * ## Thread Safety
     * 
     * - **Re-entrant**: Safe to use different stream objects from different threads
     * - **Not thread-safe**: Single stream object requires external synchronization
     * - **Destruction**: Must not destroy a stream while another thread is using it
     * - **Callbacks**: Called from audio thread, must be real-time safe
     * 
     * @note Streams must be destroyed before their parent audio_device
     * @see audio_device, audio_source, processor
     */
    class MUSAC_EXPORT audio_stream  {
        public:
            /**
             * @brief Callback function type for stream events
             */
            using callback_t = std::function <void(audio_stream&)>;

            /**
             * @brief Destructor - stops playback and cleans up resources
             */
            virtual ~audio_stream();

            /**
             * @brief Copy constructor is deleted (streams are not copyable)
             */
            audio_stream(const audio_stream&) = delete;
            
            /**
             * @brief Copy assignment is deleted (streams are not copyable)
             */
            auto operator=(const audio_stream&) -> audio_stream& = delete;

            /**
             * @brief Move constructor
             * @param other Stream to move from
             */
            audio_stream(audio_stream&& other) noexcept;
            
            /**
             * @brief Move assignment operator
             * @param other Stream to move from
             * @return Reference to this stream
             */
            auto operator=(audio_stream&& other) noexcept -> audio_stream&;
            
            /**
             * @brief Open the stream and prepare it for playback
             * 
             * Although calling this function is not required (play() will open
             * automatically), you can use it to verify the stream can be loaded
             * successfully before starting playback.
             * 
             * @throws stream_error if the stream cannot be opened
             * @throws format_error if the audio format is not supported
             * 
             * @code
             * try {
             *     stream.open();
             *     // Stream opened successfully
             * } catch (const musac::stream_error& e) {
             *     // Handle error
             * }
             * @endcode
             */
            virtual void open();

            /**
             * @brief Start playback with optional looping and fade-in
             * 
             * Begins playing the stream. If already playing, returns true without
             * restarting. The stream will automatically open if not already opened.
             * 
             * @param iterations Number of times to play (0 = loop forever, 1 = play once)
             * @param fadeTime Duration of fade-in effect
             * @return true if playback started or was already playing, false on error
             * 
             * @code
             * // Play once
             * stream.play();
             * 
             * // Play 3 times
             * stream.play(3);
             * 
             * // Loop forever with 2-second fade-in
             * using namespace std::chrono_literals;
             * stream.play(0, 2s);
             * @endcode
             * 
             * @see stop(), pause(), resume()
             */
            virtual bool play(unsigned int iterations, std::chrono::microseconds fadeTime);
            
            /**
             * @brief Play once (convenience overload)
             */
            bool play() { return play(1, std::chrono::microseconds{}); }
            
            /**
             * @brief Play with iterations (convenience overload)
             */
            bool play(unsigned int iterations) { return play(iterations, std::chrono::microseconds{}); }

            /**
             * @brief Stop playback with optional fade-out
             * 
             * Stops the stream and resets position to the beginning. If a fade
             * duration is specified, the stream continues playing while fading
             * out, then stops when the fade completes.
             * 
             * @param fade_time Duration of fade-out effect
             * 
             * @note During fade-out, is_playing() returns true until fade completes
             * 
             * @code
             * // Immediate stop
             * stream.stop();
             * 
             * // Stop with 1-second fade-out
             * using namespace std::chrono_literals;
             * stream.stop(1s);
             * @endcode
             * 
             * @see play(), pause()
             */
            virtual void stop(std::chrono::microseconds fade_time);
            
            /**
             * @brief Stop immediately (convenience overload)
             */
            void stop() { stop(std::chrono::microseconds{}); }

            /**
             * @brief Pause playback with optional fade-out
             * 
             * Temporarily suspends playback at the current position. Can be
             * resumed with resume(). Optional fade-out for smooth transitions.
             * 
             * @param fade_time Duration of fade-out effect
             * 
             * @code
             * // Immediate pause
             * stream.pause();
             * 
             * // Pause with half-second fade
             * using namespace std::chrono_literals;
             * stream.pause(500ms);
             * @endcode
             * 
             * @see resume(), stop()
             */
            virtual void pause(std::chrono::microseconds fade_time);
            
            /**
             * @brief Pause immediately (convenience overload)
             */
            void pause() { pause(std::chrono::microseconds{}); }

            /**
             * @brief Resume paused playback with optional fade-in
             * 
             * Continues playback from where it was paused. If the stream is
             * not paused, this has no effect.
             * 
             * @param fadeTime Duration of fade-in effect
             * 
             * @code
             * stream.pause();
             * // ... later ...
             * stream.resume();  // Continue from pause point
             * @endcode
             * 
             * @see pause(), play()
             */
            virtual void resume(std::chrono::microseconds fadeTime);
            
            /**
             * @brief Resume immediately (convenience overload)
             */
            void resume() { resume(std::chrono::microseconds{}); }

            /**
             * @brief Rewind stream to the beginning
             * 
             * Resets the playback position to the start of the stream.
             * 
             * @return true if rewound successfully, false if not seekable
             * 
             * @note Not all streams support rewinding (e.g., network streams)
             */
            virtual bool rewind();

            /**
             * @brief Set playback volume
             * 
             * @param volume Volume level:
             *   - 0.0 = silence
             *   - 1.0 = normal (0dB)
             *   - >1.0 = amplified (may cause distortion)
             * 
             * @code
             * stream.set_volume(0.5f);  // 50% volume
             * stream.set_volume(2.0f);  // 200% volume (amplified)
             * @endcode
             * 
             * @warning Values above 1.0 may cause clipping/distortion
             */
            virtual void set_volume(float volume);

            /**
             * @brief Get current playback volume
             * @return Current volume level (0.0 to N)
             */
            [[nodiscard]] virtual float volume() const;

            /**
             * @brief Set stereo panning position
             * 
             * Attenuates left or right channel for panning effect. Does not
             * mix channels - when panned fully right, left channel is silent.
             * 
             * @param position Pan position:
             *   - -1.0 = full left
             *   - 0.0 = center
             *   - 1.0 = full right
             * 
             * @code
             * stream.set_stereo_position(-0.5f);  // Pan 50% left
             * stream.set_stereo_position(1.0f);   // Full right
             * @endcode
             */
            virtual void set_stereo_position(float position);

            /**
             * @brief Get current stereo position
             * @return Pan position (-1.0 to 1.0)
             */
            [[nodiscard]] virtual float get_stereo_position() const;

            /**
             * @brief Mute the stream
             * 
             * Silences output while preserving volume setting. Volume can
             * still be changed while muted.
             * 
             * @see unmute(), is_muted()
             */
            virtual void mute();

            /**
             * @brief Unmute the stream
             * @see mute(), is_muted()
             */
            virtual void unmute();

            /**
             * @brief Check if stream is muted
             * @return true if muted, false otherwise
             */
            [[nodiscard]] virtual bool is_muted() const;

            /**
             * @brief Check if stream is playing
             * 
             * Returns true if playback has been started, even if paused.
             * Returns false only when stopped or not yet started.
             * 
             * @return true if playing (including paused), false if stopped
             * 
             * @note A paused stream returns true
             * @note During fade-out to stop, returns true until fade completes
             */
            [[nodiscard]] virtual bool is_playing() const;

            /**
             * @brief Check if stream is paused
             * 
             * Returns true only if explicitly paused. Stopped streams return false.
             * 
             * @return true if paused, false if playing or stopped
             */
            [[nodiscard]] virtual bool is_paused() const;

            /**
             * @brief Get stream duration
             * 
             * @return Stream duration, or zero if unknown
             * 
             * @warning Duration may be inaccurate for:
             *   - MOD files (tracker formats)
             *   - Some MP3 files (VBR without headers)
             *   - MIDI files (tempo-dependent)
             * 
             * @code
             * auto duration = stream.duration();
             * auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration);
             * std::cout << "Duration: " << seconds.count() << "s" << std::endl;
             * @endcode
             */
            [[nodiscard]] virtual std::chrono::microseconds duration() const;

            /**
             * @brief Seek to a time position
             * 
             * Changes playback position to the specified time.
             * 
             * @param pos Target position from start
             * @return true if seek succeeded, false if not supported
             * 
             * @warning Seek may be inaccurate for:
             *   - MOD files (pattern-based)
             *   - MP3 files (frame boundaries)
             *   - MIDI files (often not seekable)
             * 
             * @code
             * using namespace std::chrono_literals;
             * stream.seek_to_time(30s);  // Seek to 30 seconds
             * @endcode
             */
            virtual bool seek_to_time(std::chrono::microseconds pos);

            /**
             * @brief Set callback for playback completion
             * 
             * Called when stream stops, either manually or after finishing.
             * 
             * @param func Callback function/lambda
             * 
             * @warning Callback runs in audio thread - must be real-time safe!
             * 
             * @code
             * stream.set_finish_callback([](audio_stream& s) {
             *     std::cout << "Playback finished!" << std::endl;
             * });
             * @endcode
             */
            void set_finish_callback(callback_t func) const;

            /**
             * @brief Remove finish callback
             */
            void remove_finish_callback() const;

            /**
             * @brief Set callback for loop events
             * 
             * Called each time the stream loops when playing multiple iterations.
             * 
             * @param func Callback function/lambda
             * 
             * @warning Callback runs in audio thread - must be real-time safe!
             * 
             * @code
             * int loop_count = 0;
             * stream.set_loop_callback([&](audio_stream& s) {
             *     std::cout << "Loop " << ++loop_count << std::endl;
             * });
             * stream.play(5);  // Will call callback 4 times
             * @endcode
             */
            void set_loop_callback(callback_t func) const;

            /**
             * @brief Remove loop callback
             */
            void remove_loop_callback() const;

            /**
             * @brief Add an audio processor to the effect chain
             * 
             * Processors are applied in the order added. Each processor receives
             * the output of the previous one. Duplicate or null processors are ignored.
             * 
             * @param processor_obj Processor to add
             * 
             * @code
             * auto reverb = std::make_shared<reverb_processor>();
             * auto delay = std::make_shared<delay_processor>();
             * stream.add_processor(reverb);
             * stream.add_processor(delay);  // Applied after reverb
             * @endcode
             * 
             * @see processor, remove_processor(), clear_processors()
             */
            void add_processor(std::shared_ptr <processor> processor_obj) const;

            /**
             * @brief Remove a processor from the effect chain
             * 
             * @param processor_obj Processor to remove
             * 
             * @note Does nothing if processor not found
             */
            void remove_processor(processor* processor_obj);

            /**
             * @brief Remove all processors from the effect chain
             */
            void clear_processors();

        protected:
            /*!
             * \brief Invokes the finish-playback callback, if there is one.
             *
             * In your subclass, you must call this function whenever your stream stops playing.
             */
            void invoke_finish_callback();

            /*!
             * \brief Invokes the loop callback, if there is one.
             *
             * In your subclass, you must call this function whenever your stream loops.
             */
            void invoke_loop_callback();
            
            // State capture for device switching
            struct stream_snapshot {
                uint64_t playback_tick;
                float volume;
                float internal_volume;
                float stereo_pos;
                bool is_playing;
                bool is_paused;
                bool is_muted;
                bool starting;
                size_t current_iteration;
                size_t wanted_iterations;
                float fade_gain;
                int fade_state;
                uint64_t playback_start_tick;
            };
            
        protected:
            // Protected methods for device switching (accessible to tests and audio_mixer)
            [[nodiscard]] stream_snapshot capture_state() const;
            void restore_state(const stream_snapshot& state);
            
        private:
            friend class audio_device;
            friend class audio_mixer;  // For state capture during device switching
            friend struct in_use_guard;
            friend class audio_system;  // For device switching
            friend class pc_speaker_stream;  // For PC speaker stream creation
            
#ifdef MUSAC_BENCHMARK_MODE
        public:  // Make constructor public for benchmarking
#endif
            audio_stream(audio_source&& audio_src);
#ifdef MUSAC_BENCHMARK_MODE
        public:  // Keep public for benchmarking
#else
        private:  // Private in normal mode
#endif

            static void audio_callback(uint8_t out[], unsigned int out_len);
            static std::vector<float> get_final_output_buffer();
            static void set_audio_device_data(const audio_device_data& aud);
            [[nodiscard]] int get_token() const;
            static audio_mixer& get_global_mixer();
            
        private:
            struct impl;
            friend struct impl;
            std::unique_ptr <impl> m_pimpl;
    };
}
