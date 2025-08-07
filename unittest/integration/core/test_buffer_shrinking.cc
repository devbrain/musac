#include <doctest/doctest.h>
#include <musac/audio_mixer.hh>
#include <thread>
#include <chrono>

TEST_SUITE("Mixer::BufferManagement::Integration") {

TEST_CASE("audio_mixer buffer shrinking") {
    using namespace musac;
    
    SUBCASE("buffers grow on demand") {
        audio_mixer mixer;
        
        // Initial state
        CHECK(mixer.allocatedSamples() == 0);
        
        // Small resize
        mixer.resize(1024);
        CHECK(mixer.allocatedSamples() == 1024);
        
        // Larger resize
        mixer.resize(8192);
        CHECK(mixer.allocatedSamples() == 8192);
        
        // Even larger resize
        mixer.resize(65536);
        CHECK(mixer.allocatedSamples() == 65536);
    }
    
    SUBCASE("buffers don't shrink immediately") {
        audio_mixer mixer;
        
        // Grow to large size
        mixer.resize(300000);  // > 256K limit
        CHECK(mixer.allocatedSamples() == 300000);
        
        // Small request - should not shrink immediately
        mixer.resize(1024);
        CHECK(mixer.allocatedSamples() == 300000);
        
        // Multiple small requests - still no shrink (< 100 frames)
        for (int i = 0; i < 50; ++i) {
            mixer.resize(1024);
        }
        CHECK(mixer.allocatedSamples() == 300000);
    }
    
    SUBCASE("buffers shrink after stability period") {
        audio_mixer mixer;
        
        // Grow to large size
        mixer.resize(300000);  // > 256K limit
        unsigned int large_size = mixer.allocatedSamples();
        CHECK(large_size == 300000);
        
        // Make 101 small requests (> STABILITY_FRAMES)
        // Using < 25% of allocated size
        for (int i = 0; i < 101; ++i) {
            mixer.resize(1024);  // 1024 < 300000 * 0.25
        }
        
        // Should have shrunk after stability period
        unsigned int new_size = mixer.allocatedSamples();
        CHECK(new_size < large_size);
        CHECK(new_size <= 262144);  // MAX_RETAINED_SAMPLES
        CHECK(new_size >= 4096);     // MIN_BUFFER_SAMPLES
    }
    
    SUBCASE("buffers don't shrink if using > 25%") {
        audio_mixer mixer;
        
        // Grow to large size
        mixer.resize(300000);
        CHECK(mixer.allocatedSamples() == 300000);
        
        // Make requests using > 25% of buffer
        for (int i = 0; i < 101; ++i) {
            mixer.resize(80000);  // 80000 > 300000 * 0.25
        }
        
        // Should NOT have shrunk
        CHECK(mixer.allocatedSamples() == 300000);
    }
    
    SUBCASE("manual compaction works") {
        audio_mixer mixer;
        
        // Grow buffers
        mixer.resize(100000);
        CHECK(mixer.allocatedSamples() == 100000);
        
        // Manual compact
        mixer.compact_buffers();
        
        // Should shrink to minimum
        CHECK(mixer.allocatedSamples() == 4096);
        
        // Can grow again after compaction
        mixer.resize(8192);
        CHECK(mixer.allocatedSamples() == 8192);
    }
    
    SUBCASE("compaction only happens for large buffers") {
        audio_mixer mixer;
        
        // Small buffer
        mixer.resize(8192);
        CHECK(mixer.allocatedSamples() == 8192);
        
        // Compact - should not change small buffers
        mixer.compact_buffers();
        CHECK(mixer.allocatedSamples() == 8192);
        
        // Medium buffer (< 4 * MIN_BUFFER_SAMPLES)
        mixer.resize(16000);
        CHECK(mixer.allocatedSamples() == 16000);
        
        // Compact - still no change
        mixer.compact_buffers();
        CHECK(mixer.allocatedSamples() == 16000);
        
        // Large buffer (> 4 * MIN_BUFFER_SAMPLES)
        mixer.resize(20000);
        CHECK(mixer.allocatedSamples() == 20000);
        
        // Now compaction should work
        mixer.compact_buffers();
        CHECK(mixer.allocatedSamples() == 4096);
    }
    
    SUBCASE("shrinking resets stability counters") {
        audio_mixer mixer;
        
        // Grow large
        mixer.resize(300000);
        
        // 50 small requests
        for (int i = 0; i < 50; ++i) {
            mixer.resize(1024);
        }
        CHECK(mixer.allocatedSamples() == 300000);
        
        // One large request (resets counter)
        mixer.resize(80000);
        CHECK(mixer.allocatedSamples() == 300000);
        
        // Another 50 small requests - still not enough
        for (int i = 0; i < 50; ++i) {
            mixer.resize(1024);
        }
        CHECK(mixer.allocatedSamples() == 300000);
        
        // Need full 101 consecutive small requests
        for (int i = 0; i < 101; ++i) {
            mixer.resize(1024);
        }
        CHECK(mixer.allocatedSamples() < 300000);
    }
}

} // TEST_SUITE