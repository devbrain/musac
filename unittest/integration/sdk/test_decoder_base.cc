#include <doctest/doctest.h>
#include <musac/sdk/decoder.hh>
#include <vector>
#include <cstring>

// Test decoder implementation
class TestDecoder : public musac::decoder {
private:
    std::vector<float> m_data;
    size_t m_position = 0;
    musac::channels_t m_channels = 2;
    musac::sample_rate_t m_rate = 44100;
    
public:
    TestDecoder() = default;
    
    // Set test data
    void setTestData(const std::vector<float>& data) {
        m_data = data;
        m_position = 0;
    }
    
    void setFormat(musac::channels_t channels, musac::sample_rate_t rate) {
        m_channels = channels;
        m_rate = rate;
    }
    
    // Implement virtual functions
    void open(musac::io_stream* /*rwops*/) override {
        // Simulate opening
        set_is_open(true);
        m_position = 0;
    }
    
    musac::channels_t get_channels() const override {
        return m_channels;
    }
    
    musac::sample_rate_t get_rate() const override {
        return m_rate;
    }
    
    bool rewind() override {
        m_position = 0;
        return true;
    }
    
    std::chrono::microseconds duration() const override {
        if (m_rate == 0 || m_channels == 0) {
            return std::chrono::microseconds(0);
        }
        
        size_t frames = m_data.size() / m_channels;
        uint64_t microseconds = (frames * 1000000ULL) / m_rate;
        return std::chrono::microseconds(microseconds);
    }
    
    bool seek_to_pcm_frame(uint64_t frame_num) {
        size_t sample_pos = frame_num * m_channels;
        if (sample_pos <= m_data.size()) {
            m_position = sample_pos;
            return true;
        }
        return false;
    }
    
    bool seek_to_time(std::chrono::microseconds time) override {
        uint64_t frame_num = static_cast<uint64_t>((time.count() * m_rate) / 1000000);
        return seek_to_pcm_frame(frame_num);
    }
    
    const char* get_name() const override {
        return "Test Decoder";
    }
    
    // Static accept method for test purposes
    static bool accept(musac::io_stream* /*rwops*/) {
        // Test decoder accepts any input
        return true;
    }
    
protected:
    size_t do_decode(float* buf, size_t len, bool& call_again) override {
        size_t available = m_data.size() - m_position;
        size_t to_copy = std::min(len, available);
        
        if (to_copy > 0) {
            std::memcpy(buf, m_data.data() + m_position, to_copy * sizeof(float));
            m_position += to_copy;
        }
        
        call_again = (m_position < m_data.size());
        return to_copy;
    }
};

TEST_SUITE("SDK::DecoderBase") {
    TEST_CASE("Basic decoder functionality") {
        TestDecoder decoder;
        
        SUBCASE("Initial state") {
            CHECK_FALSE(decoder.is_open());
            CHECK(decoder.get_channels() == 2);
            CHECK(decoder.get_rate() == 44100);
            CHECK(decoder.duration() == std::chrono::microseconds(0));
        }
        
        SUBCASE("Open and close") {
            CHECK_NOTHROW(decoder.open(nullptr));
            CHECK(decoder.is_open());
            
            // Decoder doesn't have explicit close in the interface
            // set_is_open(false) is protected
        }
    }
    
    TEST_CASE("Decoding test data") {
        TestDecoder decoder;
        
        // Create test data: 1 second of stereo audio
        std::vector<float> test_data(44100 * 2);
        for (size_t i = 0; i < test_data.size(); i++) {
            test_data[i] = static_cast<float>(i) / static_cast<float>(test_data.size());
        }
        
        decoder.setTestData(test_data);
        decoder.open(nullptr);
        
        SUBCASE("Decode in chunks") {
            float buffer[1024];
            bool call_again = true;
            size_t total_decoded = 0;
            
            while (call_again) {
                size_t decoded = decoder.decode(buffer, 1024, call_again, 2);
                total_decoded += decoded;
                
                if (decoded > 0) {
                    // Verify first sample of this chunk
                    float expected = static_cast<float>(total_decoded - decoded) / static_cast<float>(test_data.size());
                    CHECK(buffer[0] == doctest::Approx(expected).epsilon(0.0001f));
                }
            }
            
            CHECK(total_decoded == test_data.size());
        }
        
        SUBCASE("Rewind") {
            float buffer[100];
            bool call_again;
            
            // Decode some data
            size_t decoded_count = decoder.decode(buffer, 100, call_again, 2);
            CHECK(decoded_count > 0);
            
            // Rewind
            CHECK(decoder.rewind());
            
            // Should decode from beginning again
            size_t decoded_count2 = decoder.decode(buffer, 100, call_again, 2);
            CHECK(decoded_count2 > 0);
            CHECK(buffer[0] == doctest::Approx(0.0f));
        }
    }
    
    TEST_CASE("Duration calculation") {
        TestDecoder decoder;
        
        SUBCASE("1 second mono") {
            decoder.setFormat(1, 44100);
            decoder.setTestData(std::vector<float>(44100));
            decoder.open(nullptr);
            
            CHECK(decoder.duration() == std::chrono::seconds(1));
        }
        
        SUBCASE("2 seconds stereo") {
            decoder.setFormat(2, 44100);
            decoder.setTestData(std::vector<float>(44100 * 2 * 2));
            decoder.open(nullptr);
            
            CHECK(decoder.duration() == std::chrono::seconds(2));
        }
        
        SUBCASE("500ms at 48kHz") {
            decoder.setFormat(1, 48000);
            decoder.setTestData(std::vector<float>(24000));
            decoder.open(nullptr);
            
            CHECK(decoder.duration() == std::chrono::milliseconds(500));
        }
    }
    
    TEST_CASE("Seeking") {
        TestDecoder decoder;
        decoder.setFormat(2, 44100);
        
        // 2 seconds of stereo data
        std::vector<float> test_data(44100 * 2 * 2);
        for (size_t i = 0; i < test_data.size(); i++) {
            test_data[i] = static_cast<float>(i);
        }
        decoder.setTestData(test_data);
        decoder.open(nullptr);
        
        SUBCASE("Seek to PCM frame") {
            // Seek to frame 22050 (0.5 seconds)
            CHECK(decoder.seek_to_pcm_frame(22050));
            
            float buffer[2];
            bool call_again;
            size_t decoded_count = decoder.decode(buffer, 2, call_again, 2);
            CHECK(decoded_count == 2);
            
            // Should be at sample 22050 * 2 = 44100
            CHECK(buffer[0] == 44100.0f);
            CHECK(buffer[1] == 44101.0f);
        }
        
        SUBCASE("Seek to time") {
            // Seek to 1.5 seconds
            CHECK(decoder.seek_to_time(std::chrono::milliseconds(1500)));
            
            float buffer[2];
            bool call_again;
            size_t decoded_count = decoder.decode(buffer, 2, call_again, 2);
            CHECK(decoded_count == 2);
            
            // Should be at frame 66150, sample 132300
            CHECK(buffer[0] == doctest::Approx(132300.0f));
        }
        
        SUBCASE("Seek beyond end") {
            CHECK_FALSE(decoder.seek_to_pcm_frame(100000));
            CHECK_FALSE(decoder.seek_to_time(std::chrono::seconds(10)));
        }
    }
    
    TEST_CASE("Error handling") {
        TestDecoder decoder;
        
        SUBCASE("Decode without data") {
            decoder.open(nullptr);
            
            float buffer[100];
            bool call_again;
            size_t decoded = decoder.decode(buffer, 100, call_again, 2);
            
            CHECK(decoded == 0);
            CHECK_FALSE(call_again);
        }
        
        SUBCASE("Empty buffer decode") {
            decoder.setTestData({1.0f, 2.0f, 3.0f});
            decoder.open(nullptr);
            
            bool call_again;
            size_t decoded = decoder.decode(nullptr, 0, call_again, 2);
            
            CHECK(decoded == 0);
            CHECK(call_again); // Still has data
        }
    }
}