#ifndef TEST_HELPERS_V2_HH
#define TEST_HELPERS_V2_HH

#include <musac/audio_system.hh>
#include <musac/audio_device.hh>
#include <musac/backends/sdl3/sdl3_backend.hh>
#include <musac/sdk/audio_backend_v2.hh>
#include <memory>

namespace musac {
namespace test {

// Helper class for tests that automatically initializes and cleans up audio system
class audio_test_fixture_v2 {
protected:
    std::shared_ptr<audio_backend_v2> backend;
    
public:
    audio_test_fixture_v2() {
        // Create and initialize SDL3 backend with dummy driver for testing
        // The dummy driver is set via environment variable in test_main.cc
        backend = std::shared_ptr<audio_backend_v2>(create_sdl3_backend_v2());
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
    std::shared_ptr<audio_backend_v2> get_backend() const {
        return backend;
    }
};

// Standalone helper functions for simple test conversions

// Initialize audio system with SDL3 backend (dummy driver) for testing
inline std::shared_ptr<audio_backend_v2> init_test_audio_system() {
    std::shared_ptr<audio_backend_v2> backend(create_sdl3_backend_v2());
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

} // namespace test
} // namespace musac

#endif // TEST_HELPERS_V2_HH