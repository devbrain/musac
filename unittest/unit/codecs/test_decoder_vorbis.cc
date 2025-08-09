#include <doctest/doctest.h>
#include <musac/codecs/decoder_vorbis.hh>
#include <musac/sdk/io_stream.hh>
#include <vector>
#include <cmath>

// Create a minimal valid Ogg Vorbis file for testing
// This is a very small silent Vorbis file
static const unsigned char minimal_vorbis[] = {
    // This would need to be a real minimal Vorbis file
    // For now, we'll just test that the decoder can be created
};

TEST_SUITE("decoder_vorbis") {
    TEST_CASE("Can create decoder") {
        musac::decoder_vorbis decoder;
        CHECK_FALSE(decoder.is_open());
    }
    
    TEST_CASE("Handles invalid data gracefully") {
        musac::decoder_vorbis decoder;
        
        // Create some invalid data
        std::vector<uint8_t> invalid_data = {0x00, 0x01, 0x02, 0x03};
        auto io = musac::io_from_memory(invalid_data.data(), invalid_data.size());
        
        // Should throw when trying to open invalid data
        CHECK_THROWS(decoder.open(io.get()));
        CHECK_FALSE(decoder.is_open());
    }
    
    TEST_CASE("Can query properties when not open") {
        musac::decoder_vorbis decoder;
        
        CHECK(decoder.get_channels() == 0);
        CHECK(decoder.get_rate() == 0);
        CHECK(decoder.duration() == std::chrono::microseconds(0));
        CHECK_FALSE(decoder.rewind());
        CHECK_FALSE(decoder.seek_to_time(std::chrono::microseconds(1000)));
    }
    
    // To properly test decoding, we would need a real Vorbis file
    // For now, these tests verify the basic structure is correct
}