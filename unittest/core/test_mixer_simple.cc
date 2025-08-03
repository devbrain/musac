#include <doctest/doctest.h>
#include <musac/audio_system.hh>
#include <musac/audio_device.hh>
#include <musac/stream.hh>
#include <musac/audio_source.hh>
#include <thread>
#include <chrono>
#include "../test_helpers.hh"

namespace musac::test {

TEST_SUITE("mixer_simple_test") {
    struct audio_test_fixture {
        audio_test_fixture() {
            audio_system::init();
        }
        
        ~audio_test_fixture() {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            audio_system::done();
        }
    };
    
    TEST_CASE_FIXTURE(audio_test_fixture, "single stream creation") {
        auto device = audio_device::open_default_device();
        // device doesn't have is_open() method
        
        device.resume();
        
        auto source = create_mock_source(44100);
        CHECK(source != nullptr);
        
        auto stream = device.create_stream(std::move(*source));
        CHECK(stream.open());
        CHECK(stream.play());
        CHECK(stream.is_playing());
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    TEST_CASE_FIXTURE(audio_test_fixture, "multiple sequential streams") {
        auto device = audio_device::open_default_device();
        device.resume();
        
        for (int i = 0; i < 5; ++i) {
            auto source = create_mock_source(44100);
            auto stream = device.create_stream(std::move(*source));
            CHECK(stream.open());
            CHECK(stream.play());
            CHECK(stream.is_playing());
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

} // namespace musac::test