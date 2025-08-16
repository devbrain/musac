#pragma once

#include <musac/sdk/types.hh>
#include <musac/sdk/io_stream.hh>
#include <musac/codecs/aiff/aiff_codec_base.hh>
#include <iff/fourcc.hh>
#include <vector>
#include <map>
#include <memory>
#include <string>

namespace musac {
namespace aiff {

// Chunk information
struct chunk_info {
    iff::fourcc id;
    uint64_t offset;  // Offset in file
    uint32_t size;    // Size of chunk data
};

// COMM chunk data
struct comm_data {
    uint16_t num_channels;
    uint32_t num_sample_frames;
    uint16_t sample_size;
    double sample_rate;
    iff::fourcc compression_type;
    std::string compression_name;
};

// SSND chunk data
struct ssnd_data {
    uint64_t data_offset;  // Offset to actual audio data
    uint32_t block_size;
    uint64_t data_size;    // Size of audio data
};

// Container parser for AIFF/AIFC files
class aiff_container {
public:
    explicit aiff_container(io_stream* io);
    ~aiff_container();
    
    // Parse the AIFF structure
    void parse();
    
    // Check if this is an AIFC (compressed) file
    bool is_aifc() const { return m_is_aifc; }
    
    // Get COMM chunk data
    const comm_data& get_comm_data() const { return m_comm; }
    
    // Get SSND chunk data  
    const ssnd_data& get_ssnd_data() const { return m_ssnd; }
    
    // Get compression type (FourCC)
    const iff::fourcc& get_compression_type() const { return m_comm.compression_type; }
    
    // Get format parameters for codec initialization
    codec_params get_codec_params() const;
    
    // Read audio samples from SSND chunk
    // Returns actual bytes read
    size_t read_audio_data(uint8_t* buffer, size_t bytes_to_read);
    
    // Seek to position in audio data (in frames)
    bool seek_to_frame(uint64_t frame_position);
    
    // Get current position in frames
    uint64_t get_current_frame() const { return m_current_frame; }
    
    // Get total number of sample frames
    uint32_t get_total_frames() const { return m_comm.num_sample_frames; }
    
    // Find a specific chunk
    const chunk_info* find_chunk(const iff::fourcc& chunk_id) const;
    
    // Read chunk data
    std::vector<uint8_t> read_chunk(const iff::fourcc& chunk_id);
    
private:
    // Parse specific chunks
    void parse_comm_chunk(const chunk_info& chunk);
    void parse_ssnd_chunk(const chunk_info& chunk);
    void parse_form_header();
    
    // Helper to read a FourCC
    iff::fourcc read_fourcc();
    
    // Helper to convert 80-bit extended float to double
    double convert_extended_to_double(const uint8_t* extended);
    
private:
    io_stream* m_io;
    bool m_is_aifc;
    
    // Chunk information
    std::map<iff::fourcc, chunk_info> m_chunks;
    
    // Parsed data
    comm_data m_comm;
    ssnd_data m_ssnd;
    
    // Current position
    uint64_t m_current_frame;
    uint64_t m_audio_data_offset;  // Absolute offset to audio data in file
};

} // namespace aiff
} // namespace musac