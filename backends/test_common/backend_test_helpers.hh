#ifndef MUSAC_BACKEND_TEST_HELPERS_HH
#define MUSAC_BACKEND_TEST_HELPERS_HH

#include <musac/sdk/audio_backend.hh>
#include <memory>
#include <string>
#include <vector>

namespace musac::test {

// Common test functions that work with any backend implementation
// These are shared between SDL2 and SDL3 backend tests

// Test backend initialization lifecycle
void test_backend_initialization(std::unique_ptr<audio_backend> backend);

// Test device enumeration
void test_device_enumeration(std::unique_ptr<audio_backend> backend);

// Test opening and closing devices
void test_device_open_close(std::unique_ptr<audio_backend> backend);

// Test device control (pause, resume, volume)
void test_device_control(std::unique_ptr<audio_backend> backend);

// Test backend capabilities
void test_backend_capabilities(std::unique_ptr<audio_backend> backend);

// Test multiple device handling
void test_multiple_devices(std::unique_ptr<audio_backend> backend);

// Test stream creation
void test_stream_creation(std::unique_ptr<audio_backend> backend);

// Test error conditions
void test_error_conditions(std::unique_ptr<audio_backend> backend);

// Helper to get a valid device ID for testing
std::string get_test_device_id(audio_backend* backend, bool for_recording = false);

// Helper to check if backend supports a feature
bool backend_supports_recording(audio_backend* backend);

} // namespace musac::test

#endif // MUSAC_BACKEND_TEST_HELPERS_HH