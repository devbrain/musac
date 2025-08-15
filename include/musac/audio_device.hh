/**
 * @file audio_device.hh
 * @brief Audio device management and stream creation
 * @author Igor
 * @date 2025-03-17
 * @ingroup devices
 */

#ifndef  AUDIO_DEVICE_HH
#define  AUDIO_DEVICE_HH

#include <vector>
#include <string>
#include <memory>
#include <musac/export_musac.h>
#include <musac/sdk/audio_format.hh>
#include <musac/sdk/types.hh>
#include <musac/sdk/audio_backend.hh>  // For device_info

namespace musac {
    // Forward declarations
    class audio_stream;
    class pc_speaker_stream;
    class audio_source;

    /**
     * @class audio_device
     * @brief Represents an audio output device for playback
     * @ingroup devices
     * 
     * The audio_device class provides the main interface for audio playback in musac.
     * It manages the connection to audio hardware and provides methods to create
     * streams for playing audio.
     * 
     * ## Basic Usage
     * 
     * @code
     * // Open the default audio device
     * auto backend = create_sdl2_backend();
     * backend->init();
     * auto device = audio_device::open_default_device(backend);
     * 
     * // Load and play an audio file
     * auto source = audio_loader::load("music.mp3");
     * auto stream = device.create_stream(std::move(source));
     * stream.play();
     * @endcode
     * 
     * ## Device Enumeration
     * 
     * @code
     * // List all available playback devices
     * auto devices = audio_device::enumerate_devices(backend, true);
     * for (const auto& info : devices) {
     *     std::cout << "Device: " << info.name 
     *               << " (ID: " << info.id << ")"
     *               << (info.is_default ? " [DEFAULT]" : "") 
     *               << std::endl;
     * }
     * @endcode
     * 
     * ## Thread Safety
     * 
     * - Device enumeration: Thread-safe
     * - Stream creation: Thread-safe  
     * - Property getters: Thread-safe
     * - Playback control: Thread-safe
     * - Device destruction: Must ensure all streams are stopped first
     * 
     * @note The device must be kept alive as long as any streams created from it
     *       are in use. Destroying the device will stop all associated streams.
     * 
     * @see audio_stream, audio_source, audio_backend
     * @since v0.1.0
     */
    class MUSAC_EXPORT audio_device {
        public:
            /**
             * @brief Enumerate available audio devices
             * 
             * Lists all audio devices available through the specified backend.
             * 
             * @param backend The audio backend to query for devices
             * @param playback_devices If true, list playback devices; if false, list recording devices
             * @return Vector of device_info structures describing available devices
             * 
             * @throws device_error if backend is not initialized
             * 
             * @code
             * auto backend = create_sdl2_backend();
             * backend->init();
             * auto devices = audio_device::enumerate_devices(backend);
             * for (const auto& dev : devices) {
             *     std::cout << dev.name << std::endl;
             * }
             * @endcode
             * 
             * @see device_info, open_device()
             */
            static std::vector<device_info> enumerate_devices(
                std::shared_ptr<audio_backend> backend,
                bool playback_devices = true);
            
            /**
             * @brief Open the default audio device
             * 
             * Opens the system's default audio output device with optional format specification.
             * 
             * @param backend The audio backend to use
             * @param spec Optional desired audio format (nullptr for backend defaults)
             * @return An audio_device object representing the opened device
             * 
             * @throws device_error if the device cannot be opened
             * @throws invalid_argument if backend is null
             * 
             * @note If spec is provided but the exact format is not supported, the device
             *       will open with the closest supported format.
             * 
             * @code
             * // Open with default format
             * auto device = audio_device::open_default_device(backend);
             * 
             * // Open with specific format
             * audio_spec spec{44100, audio_format::f32le, 2};
             * auto device = audio_device::open_default_device(backend, &spec);
             * @endcode
             * 
             * @see open_device(), enumerate_devices()
             */
            static audio_device open_default_device(
                std::shared_ptr<audio_backend> backend,
                const audio_spec* spec = nullptr);
            
            /**
             * @brief Open a specific audio device
             * 
             * Opens the audio device with the specified ID.
             * 
             * @param backend The audio backend to use
             * @param device_id Device identifier from enumerate_devices()
             * @param spec Optional desired audio format (nullptr for backend defaults)
             * @return An audio_device object representing the opened device
             * 
             * @throws device_error if the device cannot be opened
             * @throws invalid_argument if device_id is invalid
             * 
             * @code
             * auto devices = audio_device::enumerate_devices(backend);
             * if (!devices.empty()) {
             *     auto device = audio_device::open_device(backend, devices[0].id);
             * }
             * @endcode
             * 
             * @see enumerate_devices(), open_default_device()
             */
            static audio_device open_device(
                std::shared_ptr<audio_backend> backend,
                const std::string& device_id, 
                const audio_spec* spec = nullptr);
            

            /**
             * @brief Destructor
             * 
             * Closes the audio device and stops all associated streams.
             * 
             * @warning All streams created from this device will be stopped.
             */
            ~audio_device();
            
            /**
             * @brief Move constructor
             * @param other Device to move from
             */
            audio_device(audio_device&&) noexcept;

            // Device properties
            
            /**
             * @brief Get the human-readable device name
             * @return Device name string (e.g., "Speakers (Realtek Audio)")
             */
            std::string get_device_name() const;
            
            /**
             * @brief Get the device identifier
             * @return Device ID string used for opening the device
             */
            std::string get_device_id() const;
            
            /**
             * @brief Get the audio format of the device
             * @return Current audio format (e.g., s16le, f32le)
             * @see audio_format
             */
            audio_format get_format() const;
            
            /**
             * @brief Get the number of audio channels
             * @return Number of channels (1=mono, 2=stereo, etc.)
             */
            channels_t get_channels() const;
            
            /**
             * @brief Get the sample rate
             * @return Sample rate in Hz (e.g., 44100, 48000)
             */
            sample_rate_t get_freq() const;

            // Playback control
            
            /**
             * @brief Pause all streams on this device
             * @return true if successfully paused, false otherwise
             * @note Individual streams can still be controlled separately
             */
            bool pause();
            
            /**
             * @brief Check if the device is paused
             * @return true if device is paused, false otherwise
             */
            bool is_paused() const;
            
            /**
             * @brief Resume playback on this device
             * @return true if successfully resumed, false otherwise
             */
            bool resume();

            // Mute control
            
            /**
             * @brief Mute all audio output on this device
             * 
             * Uses hardware mute if available (backend support), otherwise
             * falls back to software mute in the mixer.
             * 
             * @see unmute_all(), is_all_muted(), has_hardware_mute()
             */
            void mute_all();
            
            /**
             * @brief Unmute all audio output on this device
             * @see mute_all(), is_all_muted()
             */
            void unmute_all();
            
            /**
             * @brief Check if audio is muted
             * @return true if muted, false otherwise
             */
            bool is_all_muted() const;
            
            /**
             * @brief Check if hardware mute is available
             * @return true if backend supports hardware mute, false for software fallback
             */
            bool has_hardware_mute() const;

            /**
             * @brief Get the current device gain (volume multiplier)
             * @return Gain value (1.0 = normal, >1.0 = amplified)
             */
            float get_gain() const;
            
            /**
             * @brief Set the device gain (volume multiplier)
             * @param v Gain value (1.0 = normal, 0.0 = silence, >1.0 = amplified)
             * @warning Values > 1.0 may cause distortion
             */
            void set_gain(float v);

            // Stream creation
            
            /**
             * @brief Create an audio stream from a source
             * 
             * Creates a new stream for playing audio. The source data is moved
             * into the stream, so the original source becomes invalid.
             * 
             * @param audio_src Audio source to play (will be moved)
             * @return New audio stream ready for playback
             * 
             * @code
             * auto source = audio_loader::load("music.mp3");
             * auto stream = device.create_stream(std::move(source));
             * stream.play();
             * @endcode
             * 
             * @see audio_stream, audio_source
             */
            audio_stream create_stream(audio_source&& audio_src);
            
            /**
             * @brief Create a PC speaker emulation stream
             * 
             * Creates a stream that generates square wave tones like classic
             * PC speakers. Useful for retro game sounds and MML playback.
             * 
             * @return New PC speaker stream
             * 
             * @code
             * auto pc_speaker = device.create_pc_speaker_stream();
             * pc_speaker.beep(1000.0f);  // 1kHz beep
             * pc_speaker.play_mml("T120 L4 C D E F G A B >C");  // Play scale
             * @endcode
             * 
             * @see pc_speaker_stream
             */
            pc_speaker_stream create_pc_speaker_stream();
            
            /**
             * @brief Create a stream with custom callback (advanced)
             * 
             * For advanced users who want direct control over audio generation.
             * The callback will be called from the audio thread.
             * 
             * @param callback Function called to fill audio buffer
             * @param userdata User data passed to callback
             * 
             * @warning The callback runs in real-time audio thread context.
             *          It must not block, allocate memory, or take locks.
             * 
             * @code
             * void my_callback(void* userdata, uint8_t* stream, int len) {
             *     // Fill stream with audio data
             *     memset(stream, 0, len);  // Silence for this example
             * }
             * 
             * device.create_stream_with_callback(my_callback, nullptr);
             * @endcode
             */
            void create_stream_with_callback(
                void (*callback)(void* userdata, uint8_t* stream, int len),
                void* userdata);
            
            /**
             * @brief Get the final mixed output buffer for visualization
             * 
             * Returns the last mixed audio buffer for visualization purposes
             * (e.g., waveform display, spectrum analyzer).
             * 
             * @return Vector of float samples from the last audio callback
             * 
             * @note This is a copy of the buffer, safe to use from any thread.
             * @note The buffer size depends on the device's buffer size.
             */
            std::vector<float> get_output_buffer() const;

        private:
            // Constructor with backend
            audio_device(
                std::shared_ptr<audio_backend> backend,
                const device_info& info, 
                const audio_spec* spec);
            
            // For device switching
            friend class audio_system;
            
        private:
            struct impl;
            std::unique_ptr<impl> m_pimpl;
    };
}

#endif