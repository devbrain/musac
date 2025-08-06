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



    /*!
     * \brief A \ref Stream handles playback for audio produced by a Decoder.
     *
     * All public functions of this class will lock the SDL audio device when they are called, and
     * unlock it when they return. Therefore, it is safe to manipulate a Stream that is currently
     * playing without having to manually lock the SDL audio device.
     *
     * This class is re-entrant but not thread-safe. You can call functions of this class from different
     * threads only if those calls operate on different objects. If you need to control the same Stream
     * object from multiple threads, you need to synchronize access to that object. This includes Stream
     * destruction, meaning you should not create a Stream in one thread and destroy it in another
     * without synchronization.
     */
    class MUSAC_EXPORT audio_stream  {
        public:
            using callback_t = std::function <void(audio_stream&)>;

            virtual ~audio_stream();

            audio_stream(const audio_stream&) = delete;
            auto operator=(const audio_stream&) -> audio_stream& = delete;

            audio_stream(audio_stream&& other) noexcept;
            auto operator=(audio_stream&& other) noexcept -> audio_stream&;
            /*!
             * \brief Open the stream and prepare it for playback.
             *
             * Although calling this function is not required, you can use it in order to determine whether
             * the stream can be loaded successfully prior to starting playback.
             *
             * \throws musac_error if the stream cannot be opened.
             */
            virtual void open();

            /*!
             * \brief Start playback.
             *
             * \param iterations
             *  The amount of times the stream should be played. If zero, the stream will loop forever.
             *
             * \param fadeTime
             *  Fade-in over the specified amount of time.
             *
             * \return
             *  \retval true Playback was started successfully, or it was already started.
             *  \retval false Playback could not be started.
             */
            virtual bool play(unsigned int iterations = 1, std::chrono::microseconds fadeTime = {});

            /*!
             * \brief Stop playback.
             *
             * When calling this, the stream is reset to the beginning again.
             *
             * \param fade_time
             *  Fade-out over the specified amount of time.
             */
            virtual void stop(std::chrono::microseconds fade_time = {});

            /*!
             * \brief Pause playback.
             *
             * \param fade_time
             *  Fade-out over the specified amount of time.
             */
            virtual void pause(std::chrono::microseconds fade_time = {});

            /*!
             * \brief Resume playback.
             *
             * \param fadeTime
             *  Fade-in over the specified amount of time.
             */
            virtual void resume(std::chrono::microseconds fadeTime = {});

            /*!
             * \brief Rewind stream to the beginning.
             *
             * \return
             *  \retval true Stream was rewound successfully.
             *  \retval false Stream could not be rewound.
             */
            virtual bool rewind();

            /*!
             * \brief Change playback volume.
             *
             * \param volume
             *  0.0 means total silence, while 1.0 means non-attenuated, 100% (0db) volume. Values above 1.0
             *  are possible and will result in gain being applied (which might result in distortion.) For
             *  example, 3.5 would result in 350% percent volume. There is no upper limit.
             */
            virtual void set_volume(float volume);

            /*!
             * \brief Get current playback volume.
             *
             * \return
             *  Current playback volume.
             */
            [[nodiscard]] virtual float volume() const;

            /*!
             * \brief Set stereo position.
             *
             * This only attenuates the left or right channel. It does not mix one into the other. For
             * example, when setting the position of a stereo stream all the way to the right, the left
             * channel will be completely inaudible. It will not be mixed into the right channel.
             *
             * \param position
             *  Must be between -1.0 (all the way to the left) and 1.0 (all the way to the right) with 0
             *  being the center position.
             */
            virtual void set_stereo_position(float position);

            /*!
             * \brief Returns the currently set stereo position.
             */
            [[nodiscard]] virtual float get_stereo_position() const;

            /*!
             * \brief Mute the stream.
             *
             * A muted stream still accepts volume changes, but it will stay inaudible until it is unmuted.
             */
            virtual void mute();

            /*!
             * \brief Unmute the stream.
             */
            virtual void unmute();

            /*!
             * \brief Returns true if the stream is muted, false otherwise.
             */
            [[nodiscard]] virtual bool is_muted() const;

            /*!
             * \brief Get current playback state.
             *
             * Note that a paused stream is still considered as being in the playback state.
             *
             * \return
             *  \retval true Playback has been started.
             *  \retval false Playback has not been started yet, or was stopped.
             */
            [[nodiscard]] virtual bool is_playing() const;

            /*!
             * \brief Get current pause state.
             *
             * Note that a stream that is not in playback state is not considered paused. This will return
             * false even for streams where playback is stopped.
             *
             * \return
             *  \retval true The stream is currently paused.
             *  \retval false The stream is currently not paused.
             */
            [[nodiscard]] virtual bool is_paused() const;

            /*!
             * \brief Get stream duration.
             *
             * It is possible that for some streams (for example MOD files and some MP3 files), the reported
             * duration can be wrong. For some streams, it might not even be possible to get a duration at
             * all (MIDI files, for example.)
             *
             * \return
             * Stream duration. If the stream does not provide duration information, a zero duration is
             * returned.
             */
            [[nodiscard]] virtual std::chrono::microseconds duration() const;

            /*!
             * \brief Seek to a time position in the stream.
             *
             * This will change the current playback position in the stream to the specified time.
             *
             * Note that for some streams (for example MOD files and some MP3 files), it might not be
             * possible to do an accurate seek, in which case the position that is set might be off by
             * some margin. In some streams, seeking is not possible at all (MIDI files, for example.)
             *
             * \param pos
             *  Position to seek to.
             *
             * \return
             *  \retval true The playback position was changed successfully.
             *  \retval false This stream does not support seeking.
             */
            virtual bool seek_to_time(std::chrono::microseconds pos);

            /*!
             * \brief Set a callback for when the stream finishes playback.
             *
             * The callback will be called when the stream finishes playing. This can happen when you
             * manually stop it, or when it finishes playing on its own.
             *
             * \param func
             *  Anything that can be wrapped by an std::function (like a function pointer, functor or
             *  lambda.)
             */
            void set_finish_callback(callback_t func) const;

            /*!
             * \brief Removes any finish-playback callback that was previously set.
             */
            void remove_finish_callback() const;

            /*!
             * \brief Set a callback for when the stream loops.
             *
             * The callback will be called when the stream loops. It will be called once per loop.
             *
             * \param func
             *  Anything that can be wrapped by an std::function (like a function pointer, functor or
             *  lambda.)
             */
            void set_loop_callback(callback_t func) const;

            /*!
             * \brief Removes any loop callback that was previously set.
             */
            void remove_loop_callback() const;

            /*!
             * \brief Add an audio processor to the bottom of the processor list.
             *
             * You can add multiple processors. They will be run in the order they were added, each one
             * using the previous processor's output as input. If the processor instance already exists in
             * the processor list, or is a nullptr, the function does nothing.
             *
             * \param processor_obj The processor to add.
             */
            void add_processor(std::shared_ptr <processor> processor_obj) const;

            /*!
             * \brief Remove a processor from the stream.
             *
             * If the processor instance is not found, the function does nothing.
             *
             * \param processor_obj Processor to remove.
             */
            void remove_processor(processor* processor_obj);

            /*!
             * \brief Remove all processors from the stream.
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
            static void set_audio_device_data(const audio_device_data& aud);
            [[nodiscard]] int get_token() const;
            static audio_mixer& get_global_mixer();
            
        private:
            struct impl;
            friend struct impl;
            std::unique_ptr <impl> m_pimpl;
    };
}
