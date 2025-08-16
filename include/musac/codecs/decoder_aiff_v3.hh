#pragma once

#include <musac/sdk/decoder.hh>
#include <musac/codecs/export_musac_codecs.h>
#include <memory>
#include <vector>

namespace musac {

// Forward declarations
namespace aiff {
    class aiff_container;
    class aiff_codec_base;
}

/**
 * Refactored AIFF/AIFC decoder
 * 
 * This is the new modular implementation that replaces decoder_aiff_v2.
 * It acts as an orchestrator between the container parser and compression codecs.
 */
class MUSAC_CODECS_EXPORT decoder_aiff_v3 : public decoder {
public:
    decoder_aiff_v3();
    ~decoder_aiff_v3() override;
    
    // Decoder interface
    void open(io_stream* io) override;
    const char* get_name() const override;
    
    sample_rate_t get_rate() const override;
    channels_t get_channels() const override;
    std::chrono::microseconds duration() const override;
    
    bool rewind() override;
    bool seek_to_time(std::chrono::microseconds time) override;
    
    // AIFF-specific methods
    bool is_compressed() const;
    const char* get_compression_name() const;
    
    // Accept method for format detection
    static bool accept(io_stream* io);
    
protected:
    // Decoder interface
    size_t do_decode(float* buf, size_t len, bool& call_again) override;
    
private:
    class impl;
    std::unique_ptr<impl> m_pimpl;
};

} // namespace musac