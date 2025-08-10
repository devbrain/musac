/**
 * WebAssembly bindings for Musac decoder library
 * Uses Emscripten's Embind to expose C++ classes to JavaScript
 */

#include <emscripten/bind.h>
#include <emscripten/val.h>
#include <musac/sdk/decoder.hh>
#include <musac/sdk/decoders_registry.hh>
#include <musac/sdk/io_stream.hh>
#include <musac/sdk/audio_format.hh>
#include <musac/codecs/register_codecs.hh>
#include <memory>
#include <vector>

using namespace emscripten;

// JavaScript-compatible io_stream implementation
class JSArrayStream : public musac::io_stream {
private:
    std::vector<musac::uint8_t> data;
    musac::size position;
    bool open_flag;
    
public:
    explicit JSArrayStream(const std::vector<musac::uint8_t>& bytes)
        : data(bytes), position(0), open_flag(true) {}
    
    musac::size read(void* ptr, musac::size size_bytes) override {
        if (!open_flag || position >= data.size()) {
            return 0;
        }
        
        musac::size available = data.size() - position;
        musac::size to_read = std::min(size_bytes, available);
        
        std::memcpy(ptr, data.data() + position, to_read);
        position += to_read;
        
        return to_read;
    }
    
    musac::size write(const void* ptr, musac::size size_bytes) override {
        return 0; // Read-only
    }
    
    musac::int64_t seek(musac::int64_t offset, musac::seek_origin whence) override {
        if (!open_flag) return -1;
        
        musac::int64_t new_pos;
        switch (whence) {
            case musac::seek_origin::set:
                new_pos = offset;
                break;
            case musac::seek_origin::cur:
                new_pos = static_cast<musac::int64_t>(position) + offset;
                break;
            case musac::seek_origin::end:
                new_pos = static_cast<musac::int64_t>(data.size()) + offset;
                break;
            default:
                return -1;
        }
        
        if (new_pos < 0 || new_pos > static_cast<musac::int64_t>(data.size())) {
            return -1;
        }
        
        position = static_cast<musac::size>(new_pos);
        return new_pos;
    }
    
    musac::int64_t tell() override {
        return open_flag ? static_cast<musac::int64_t>(position) : -1;
    }
    
    musac::int64_t get_size() override {
        return open_flag ? static_cast<musac::int64_t>(data.size()) : -1;
    }
    
    void close() override {
        open_flag = false;
    }
    
    bool is_open() const override {
        return open_flag;
    }
};

// Global decoder registry
static std::shared_ptr<musac::decoders_registry> g_registry;

// Initialize the library
void init_musac() {
    if (!g_registry) {
        g_registry = musac::create_registry_with_all_codecs();
    }
}

// Detect audio format from data
std::string detect_format(const std::vector<musac::uint8_t>& data) {
    if (!g_registry) {
        init_musac();
    }
    
    auto stream = std::make_unique<JSArrayStream>(data);
    auto decoder = g_registry->find_decoder(stream.get());
    
    if (decoder) {
        return std::string(decoder->get_name());
    }
    return "";
}

// JavaScript-friendly decoder wrapper
class JSDecoder {
private:
    std::unique_ptr<musac::decoder> decoder;
    std::unique_ptr<JSArrayStream> stream;
    musac::channels_t channels;
    musac::sample_rate_t sample_rate;
    
public:
    JSDecoder(const std::vector<musac::uint8_t>& data) {
        if (!g_registry) {
            init_musac();
        }
        
        // Create stream from data
        stream = std::make_unique<JSArrayStream>(data);
        
        // Find and create decoder
        decoder = g_registry->find_decoder(stream.get());
        if (!decoder) {
            throw std::runtime_error("Could not detect audio format");
        }
        
        // Reset stream and open decoder
        stream->seek(0, musac::seek_origin::set);
        decoder->open(stream.get());
        
        // Cache format info
        channels = decoder->get_channels();
        sample_rate = decoder->get_rate();
    }
    
    ~JSDecoder() = default;
    
    int get_channels() const {
        return channels;
    }
    
    int get_sample_rate() const {
        return sample_rate;
    }
    
    std::string get_name() const {
        return decoder ? decoder->get_name() : "";
    }
    
    double get_duration() const {
        if (!decoder) return 0;
        auto duration = decoder->duration();
        return duration.count() / 1000000.0; // Convert to seconds
    }
    
    // Decode samples as float array
    val decode_float(int num_samples) {
        if (!decoder) {
            return val::array();
        }
        
        std::vector<float> buffer(num_samples);
        bool call_again = false;
        size_t decoded = decoder->decode(buffer.data(), num_samples, call_again, channels);
        
        if (decoded == 0) {
            return val::array();
        }
        
        // Convert to JavaScript Float32Array
        buffer.resize(decoded);
        return val(typed_memory_view(decoded, buffer.data()));
    }
    
    // Decode samples as 16-bit integer array
    val decode_int16(int num_samples) {
        if (!decoder) {
            return val::array();
        }
        
        std::vector<float> float_buffer(num_samples);
        bool call_again = false;
        size_t decoded = decoder->decode(float_buffer.data(), num_samples, call_again, channels);
        
        if (decoded == 0) {
            return val::array();
        }
        
        // Convert float to int16_t
        std::vector<int16_t> int_buffer(decoded);
        for (size_t i = 0; i < decoded; i++) {
            float sample = float_buffer[i];
            if (sample > 1.0f) sample = 1.0f;
            if (sample < -1.0f) sample = -1.0f;
            int_buffer[i] = static_cast<int16_t>(sample * 32767.0f);
        }
        
        return val(typed_memory_view(decoded, int_buffer.data()));
    }
    
    // Decode entire file
    val decode_all_float() {
        std::vector<float> all_samples;
        const int chunk_size = 4096;
        
        while (true) {
            std::vector<float> buffer(chunk_size);
            bool call_again = false;
            size_t decoded = decoder->decode(buffer.data(), chunk_size, call_again, channels);
            
            if (decoded == 0) break;
            
            all_samples.insert(all_samples.end(), buffer.begin(), buffer.begin() + decoded);
        }
        
        return val(typed_memory_view(all_samples.size(), all_samples.data()));
    }
    
    void seek(double seconds) {
        if (decoder) {
            auto microseconds = std::chrono::microseconds(
                static_cast<std::chrono::microseconds::rep>(seconds * 1000000));
            decoder->seek_to_time(microseconds);
        }
    }
    
    void rewind() {
        if (decoder) {
            decoder->rewind();
        }
    }
};

// Check if file extension is supported
bool can_decode_extension(const std::string& ext) {
    static const std::vector<std::string> supported = {
        "wav", "mp3", "ogg", "flac", "aiff", "aif",
        "mod", "s3m", "xm", "it", 
        "mid", "midi", "mus", "xmi", "hmi", "hmp",
        "voc", "vgm", "vgz", "cmf", "opb", "mml"
    };
    
    std::string lower_ext = ext;
    std::transform(lower_ext.begin(), lower_ext.end(), lower_ext.begin(), ::tolower);
    
    return std::find(supported.begin(), supported.end(), lower_ext) != supported.end();
}

// Embind bindings
EMSCRIPTEN_BINDINGS(musac_module) {
    // Register vector types for data transfer
    register_vector<uint8_t>("Uint8Vector");
    register_vector<float>("FloatVector");
    register_vector<int16_t>("Int16Vector");
    
    // Global functions
    function("init", &init_musac);
    function("detectFormat", &detect_format);
    function("canDecodeExtension", &can_decode_extension);
    
    // Decoder class
    class_<JSDecoder>("Decoder")
        .constructor<const std::vector<musac::uint8_t>&>()
        .function("getChannels", &JSDecoder::get_channels)
        .function("getSampleRate", &JSDecoder::get_sample_rate)
        .function("getName", &JSDecoder::get_name)
        .function("getDuration", &JSDecoder::get_duration)
        .function("decodeFloat", &JSDecoder::decode_float)
        .function("decodeInt16", &JSDecoder::decode_int16)
        .function("decodeAllFloat", &JSDecoder::decode_all_float)
        .function("seek", &JSDecoder::seek)
        .function("rewind", &JSDecoder::rewind);
}