#ifndef MUSAC_MOCK_BACKENDS_HH
#define MUSAC_MOCK_BACKENDS_HH

#include <musac/sdk/audio_backend.hh>
#include <musac/sdk/audio_stream_interface.hh>
#include <musac/sdk/audio_format.hh>
#include <memory>
#include <vector>
#include <map>
#include <functional>
#include <atomic>
#include <mutex>
#include <cstring>  // for std::memcpy
#include <algorithm>  // for std::min
#include <stdexcept>  // for std::runtime_error

namespace musac::test {

    // Mock stream implementation for testing
    class mock_stream : public audio_stream_interface {
        private:
            audio_spec m_spec;
            bool m_paused{true};
            std::vector<uint8_t> m_buffer;
            std::mutex m_mutex;
            size_t m_queued_size{0};

        public:
            // Statistics for testing
            std::atomic<int> put_data_calls{0};
            std::atomic<int> get_data_calls{0};
            std::atomic<int> clear_calls{0};
            std::atomic<int> pause_calls{0};
            std::atomic<int> resume_calls{0};

            // Configurable behaviors
            std::function<bool(const void*, size_t)> on_put_data;
            std::function<size_t(void*, size_t)> on_get_data;
            std::function<void()> on_clear;
            std::function<bool()> on_pause;
            std::function<bool()> on_resume;

            explicit mock_stream(const audio_spec& spec) : m_spec(spec) {}

            bool put_data(const void* data, size_t size) override {
                put_data_calls++;
                if (on_put_data) {
                    return on_put_data(data, size);
                }

                std::lock_guard<std::mutex> lock(m_mutex);
                const auto* bytes = static_cast<const uint8_t*>(data);
                m_buffer.insert(m_buffer.end(), bytes, bytes + size);
                m_queued_size += size;
                return true;
            }

            size_t get_data(void* data, size_t size) override {
                get_data_calls++;
                if (on_get_data) {
                    return on_get_data(data, size);
                }

                std::lock_guard<std::mutex> lock(m_mutex);
                size_t to_copy = std::min(size, m_buffer.size());
                if (to_copy > 0 && data) {
                    std::memcpy(data, m_buffer.data(), to_copy);
                    m_buffer.erase(m_buffer.begin(), m_buffer.begin() + static_cast<std::vector<uint8_t>::difference_type>(to_copy));
                    m_queued_size -= to_copy;
                }
                return to_copy;
            }

            void clear() override {
                clear_calls++;
                if (on_clear) {
                    on_clear();
                    return;
                }

                std::lock_guard<std::mutex> lock(m_mutex);
                m_buffer.clear();
                m_queued_size = 0;
            }

            bool pause() override {
                pause_calls++;
                if (on_pause) {
                    return on_pause();
                }
                m_paused = true;
                return true;
            }

            bool resume() override {
                resume_calls++;
                if (on_resume) {
                    return on_resume();
                }
                m_paused = false;
                return true;
            }

            bool is_paused() const override {
                return m_paused;
            }

            size_t get_queued_size() const override {
                std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(m_mutex));
                return m_queued_size;
            }

            bool bind_to_device() override { return true; }
            void unbind_from_device() override {}

            // Test helper to reset statistics
            void reset_stats() {
                put_data_calls = 0;
                get_data_calls = 0;
                clear_calls = 0;
                pause_calls = 0;
                resume_calls = 0;
            }
    };

    // Enhanced mock backend with configurable behavior
    class mock_backend_v2_enhanced : public audio_backend {
        private:
            bool m_initialized{false};
            std::map<uint32_t, device_info_v2> m_devices;
            std::map<uint32_t, audio_spec> m_device_specs;
            std::map<uint32_t, float> m_device_gains;
            std::map<uint32_t, bool> m_device_paused;
            uint32_t m_next_handle{1};
            mutable std::mutex m_mutex;

        public:
            // Statistics for testing
            std::atomic<int> init_calls{0};
            std::atomic<int> shutdown_calls{0};
            std::atomic<int> enumerate_calls{0};
            std::atomic<int> open_device_calls{0};
            std::atomic<int> close_device_calls{0};
            std::atomic<int> create_stream_calls{0};

            // Configurable behaviors
            std::function<void()> on_init;
            std::function<void()> on_shutdown;
            std::function<std::vector<device_info_v2>(bool)> on_enumerate;
            std::function<uint32_t(const std::string&, const audio_spec*)> on_open_device;
            std::function<void(uint32_t)> on_close_device;
            std::function<std::unique_ptr<audio_stream_interface>(uint32_t, const audio_spec&, void(*)(void*, uint8_t*, int), void*)> on_create_stream;

            // Error injection
            bool fail_init{false};
            bool fail_enumerate{false};
            bool fail_open_device{false};
            bool fail_create_stream{false};

            mock_backend_v2_enhanced() {
                // Setup default test devices
                device_info_v2 default_device;
                default_device.id = "mock_default";
                default_device.name = "Mock Default Device";
                default_device.channels = 2;
                default_device.sample_rate = 44100;
                default_device.is_default = true;
                // is_capture not in v2 API
                m_devices[0] = default_device;

                device_info_v2 secondary_device;
                secondary_device.id = "mock_secondary";
                secondary_device.name = "Mock Secondary Device";
                secondary_device.channels = 2;
                secondary_device.sample_rate = 48000;
                secondary_device.is_default = false;
                // is_capture not in v2 API
                m_devices[1] = secondary_device;
            }

            void init() override {
                init_calls++;
                if (fail_init) {
                    throw std::runtime_error("Mock backend init failed");
                }
                if (on_init) {
                    on_init();
                }
                m_initialized = true;
            }

            void shutdown() override {
                shutdown_calls++;
                if (on_shutdown) {
                    on_shutdown();
                }
                m_initialized = false;
                m_device_specs.clear();
                m_device_gains.clear();
                m_device_paused.clear();
            }

            bool is_initialized() const override {
                return m_initialized;
            }

            std::string get_name() const override {
                return "mock_backend_v2_enhanced";
            }

            bool supports_recording() const override {
                return false;  // Mock backend doesn't support recording
            }

            int get_max_open_devices() const override {
                return 16;  // Arbitrary limit for testing
            }

            std::vector<device_info_v2> enumerate_devices(bool playback) override {
                enumerate_calls++;
                if (!m_initialized) {
                    throw std::runtime_error("Backend not initialized");
                }
                if (fail_enumerate) {
                    throw std::runtime_error("Mock enumerate failed");
                }
                if (on_enumerate) {
                    return on_enumerate(playback);
                }

                std::vector<device_info_v2> result;
                // Since v2 API doesn't have is_capture, we return all devices for playback
                // and empty for recording
                if (playback) {
                    for (const auto& [id, device] : m_devices) {
                        result.push_back(device);
                    }
                }
                return result;
            }

            device_info_v2 get_default_device(bool playback) override {
                if (!m_initialized) {
                    throw std::runtime_error("Backend not initialized");
                }

                // Since v2 API doesn't have is_capture, we just check for default
                if (playback) {
                    for (const auto& [id, device] : m_devices) {
                        if (device.is_default) {
                            return device;
                        }
                    }
                }
                throw std::runtime_error("No default device found");
            }

            uint32_t open_device(const std::string& device_id,
                                 const audio_spec& spec,
                                 audio_spec& obtained_spec) override {
                open_device_calls++;
                if (!m_initialized) {
                    throw std::runtime_error("Backend not initialized");
                }
                if (fail_open_device) {
                    throw std::runtime_error("Mock open device failed");
                }
                if (on_open_device) {
                    // Note: on_open_device signature might need updating
                    return on_open_device(device_id, &spec);
                }

                std::lock_guard<std::mutex> lock(m_mutex);
                uint32_t handle = m_next_handle++;

                // Find device info
                for (const auto& [id, device] : m_devices) {
                    if (device.id == device_id) {
                        // Store the requested spec
                        m_device_specs[handle] = spec;
                        // Return the same spec as obtained (mock accepts any spec)
                        obtained_spec = spec;

                        m_device_gains[handle] = 1.0f;
                        m_device_paused[handle] = true;
                        return handle;
                    }
                }

                throw std::runtime_error("Device not found: " + device_id);
            }

            void close_device(uint32_t device_handle) override {
                close_device_calls++;
                if (on_close_device) {
                    on_close_device(device_handle);
                }

                std::lock_guard<std::mutex> lock(m_mutex);
                m_device_specs.erase(device_handle);
                m_device_gains.erase(device_handle);
                m_device_paused.erase(device_handle);
            }


            uint8_t get_device_channels(uint32_t device_handle) override {
                std::lock_guard<std::mutex> lock(m_mutex);
                auto it = m_device_specs.find(device_handle);
                return it != m_device_specs.end() ? it->second.channels : 0;
            }

            uint32_t get_device_frequency(uint32_t device_handle) override {
                std::lock_guard<std::mutex> lock(m_mutex);
                auto it = m_device_specs.find(device_handle);
                return it != m_device_specs.end() ? it->second.freq : 0;
            }

            audio_format get_device_format(uint32_t device_handle) override {
                std::lock_guard<std::mutex> lock(m_mutex);
                auto it = m_device_specs.find(device_handle);
                return it != m_device_specs.end() ? it->second.format : audio_format::unknown;
            }

            float get_device_gain(uint32_t device_handle) override {
                std::lock_guard<std::mutex> lock(m_mutex);
                auto it = m_device_gains.find(device_handle);
                return it != m_device_gains.end() ? it->second : 1.0f;
            }

            void set_device_gain(uint32_t device_handle, float gain) override {
                std::lock_guard<std::mutex> lock(m_mutex);
                if (m_device_gains.count(device_handle)) {
                    // Clamp gain to [0.0, 1.0] range
                    gain = std::max(0.0f, std::min(1.0f, gain));
                    m_device_gains[device_handle] = gain;
                }
            }

            bool pause_device(uint32_t device_handle) override {
                std::lock_guard<std::mutex> lock(m_mutex);
                if (m_device_paused.count(device_handle)) {
                    m_device_paused[device_handle] = true;
                    return true;
                }
                return false;
            }

            bool resume_device(uint32_t device_handle) override {
                std::lock_guard<std::mutex> lock(m_mutex);
                if (m_device_paused.count(device_handle)) {
                    m_device_paused[device_handle] = false;
                    return true;
                }
                return false;
            }

            bool is_device_paused(uint32_t device_handle) override {
                std::lock_guard<std::mutex> lock(m_mutex);
                auto it = m_device_paused.find(device_handle);
                return it != m_device_paused.end() ? it->second : true;
            }

            std::unique_ptr<audio_stream_interface> create_stream(
                uint32_t device_handle,
                const audio_spec& spec,
                void (*callback)(void* userdata, uint8_t* stream, int len),
                void* userdata) override {

                create_stream_calls++;
                if (fail_create_stream) {
                    throw std::runtime_error("Mock create stream failed");
                }
                if (on_create_stream) {
                    return on_create_stream(device_handle, spec, callback, userdata);
                }

                return std::make_unique<mock_stream>(spec);
            }

            // Test helpers
            void reset_stats() {
                init_calls = 0;
                shutdown_calls = 0;
                enumerate_calls = 0;
                open_device_calls = 0;
                close_device_calls = 0;
                create_stream_calls = 0;
            }

            void add_test_device(const device_info_v2& device) {
                std::lock_guard<std::mutex> lock(m_mutex);
                m_devices[static_cast<uint32_t>(m_devices.size())] = device;
            }

            void clear_test_devices() {
                std::lock_guard<std::mutex> lock(m_mutex);
                m_devices.clear();
            }
    };

    // Factory functions for creating mocks with specific behaviors
    inline std::shared_ptr<mock_backend_v2_enhanced> create_failing_backend(
        bool fail_init = false,
        bool fail_enumerate = false,
        bool fail_open_device = false,
        bool fail_create_stream = false) {

        auto backend = std::make_shared<mock_backend_v2_enhanced>();
        backend->fail_init = fail_init;
        backend->fail_enumerate = fail_enumerate;
        backend->fail_open_device = fail_open_device;
        backend->fail_create_stream = fail_create_stream;
        return backend;
    }

    inline std::shared_ptr<mock_backend_v2_enhanced> create_backend_with_devices(
        const std::vector<device_info_v2>& devices) {

        auto backend = std::make_shared<mock_backend_v2_enhanced>();
        backend->clear_test_devices();
        for (const auto& device : devices) {
            backend->add_test_device(device);
        }
        return backend;
    }

}

#endif // MUSAC_MOCK_BACKENDS_HH
