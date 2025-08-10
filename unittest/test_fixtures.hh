#ifndef MUSAC_TEST_FIXTURES_HH
#define MUSAC_TEST_FIXTURES_HH

#include <musac/audio_system.hh>
#include <musac/audio_device.hh>
#include <musac/stream.hh>
#include <musac_backends/sdl3/sdl3_backend.hh>
#include <musac/sdk/audio_backend.hh>
#include <memory>
#include <thread>
#include <chrono>
#include <vector>
#include <utility>
#include "mock_components.hh"  // For create_mock_source
#include <doctest/doctest.h>  // For CHECK macro

namespace musac::test {
    // Helper class for tests that automatically initializes and cleans up audio system
    class audio_test_fixture_v2 {
        protected:
            std::shared_ptr <audio_backend> backend;

        public:
            audio_test_fixture_v2() {
                // Create and initialize SDL3 backend with dummy driver for testing
                // The dummy driver is set via environment variable in test_main.cc
                backend = std::shared_ptr <audio_backend>(create_sdl3_backend());
                audio_system::init(backend);
            }

            ~audio_test_fixture_v2() {
                audio_system::done();
            }

            // Helper to create a device with default settings
            audio_device create_default_device() {
                return audio_device::open_default_device(backend);
            }

            // Helper to create a device with specific settings
            audio_device create_device_with_spec(const audio_spec& spec) {
                return audio_device::open_default_device(backend, &spec);
            }

            // Helper to get the backend
            std::shared_ptr <audio_backend> get_backend() const {
                return backend;
            }
    };

    // Thread-safe test fixture that adds delay in destructor for proper cleanup
    // This is commonly used in thread safety tests to ensure callbacks complete
    class audio_test_fixture_threadsafe : public audio_test_fixture_v2 {
        public:
            ~audio_test_fixture_threadsafe() {
                // Give time for any running callbacks to complete
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
    };

    // RAII wrapper for backend initialization/shutdown
    class backend_guard {
        private:
            std::shared_ptr <audio_backend> m_backend;

        public:
            explicit backend_guard(std::shared_ptr <audio_backend> backend = nullptr)
                : m_backend(backend ? backend : std::shared_ptr <audio_backend>(create_sdl3_backend())) {
                if (m_backend) {
                    m_backend->init();
                }
            }

            ~backend_guard() {
                if (m_backend && m_backend->is_initialized()) {
                    m_backend->shutdown();
                }
            }

            // Disable copy
            backend_guard(const backend_guard&) = delete;
            backend_guard& operator=(const backend_guard&) = delete;

            // Enable move
            backend_guard(backend_guard&& other) noexcept
                : m_backend(std::move(other.m_backend)) {
            }

            backend_guard& operator=(backend_guard&& other) noexcept {
                if (this != &other) {
                    if (m_backend && m_backend->is_initialized()) {
                        m_backend->shutdown();
                    }
                    m_backend = std::move(other.m_backend);
                }
                return *this;
            }

            std::shared_ptr <audio_backend> get() const { return m_backend; }
            audio_backend* operator->() const { return m_backend.get(); }
            audio_backend& operator*() const { return *m_backend; }
    };

    // Standalone helper functions for simple test conversions

    // Initialize audio system with SDL3 backend (dummy driver) for testing
    inline std::shared_ptr <audio_backend> init_test_audio_system() {
        std::shared_ptr <audio_backend> backend(create_sdl3_backend());
        audio_system::init(backend);
        return backend;
    }

    // Create a default test device
    inline audio_device create_test_device() {
        auto backend = audio_system::get_backend();
        if (!backend) {
            backend = init_test_audio_system();
        }
        return audio_device::open_default_device(backend);
    }

    // Create a test device with specific spec
    inline audio_device create_test_device(const audio_spec& spec) {
        auto backend = audio_system::get_backend();
        if (!backend) {
            backend = init_test_audio_system();
        }
        return audio_device::open_default_device(backend, &spec);
    }

    // Helper to create a playing stream with mock source
    inline std::unique_ptr <audio_stream> create_playing_stream(audio_device& device, size_t duration_samples = 44100) {
        auto source = create_mock_source(duration_samples);
        auto stream = std::make_unique <audio_stream>(device.create_stream(std::move(*source)));
        stream->open();
        stream->play();
        return stream;
    }

    // Helper to create device with stream ready to play
    inline std::pair <audio_device, std::unique_ptr <audio_stream>> create_device_with_stream(
        std::shared_ptr <audio_backend> backend = nullptr,
        size_t duration_samples = 44100) {
        if (!backend) {
            backend = std::shared_ptr <audio_backend>(create_sdl3_backend());
            backend->init();
        }

        auto device = audio_device::open_default_device(backend);
        device.resume();

        auto stream = create_playing_stream(device, duration_samples);
        return {std::move(device), std::move(stream)};
    }

    // Note: Stream state verification helper removed as audio_stream doesn't expose get_state() method

    // Helper to run concurrent operations on a device
    template<typename Operation>
    inline void run_concurrent_test(Operation op, int thread_count, int operations_per_thread = 1) {
        std::vector <std::thread> threads;
        threads.reserve(static_cast <size_t>(thread_count));

        for (int i = 0; i < thread_count; ++i) {
            threads.emplace_back([&op, operations_per_thread]() {
                for (int j = 0; j < operations_per_thread; ++j) {
                    op();
                }
            });
        }

        for (auto& t : threads) {
            t.join();
        }
    }
} // namespace musac::test

#endif // MUSAC_TEST_FIXTURES_HH
