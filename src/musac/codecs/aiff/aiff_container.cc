#include <musac/codecs/aiff/aiff_container.hh>
#include <musac/error.hh>
#include <iff/endian.hh>
#include <cstring>
#include <stdexcept>
#include <algorithm>
#include <cmath>
#include <iostream>

namespace musac {
namespace aiff {

// AIFF chunk identifiers
static const iff::fourcc FORM_ID("FORM");
static const iff::fourcc AIFF_ID("AIFF");
static const iff::fourcc AIFC_ID("AIFC");
static const iff::fourcc COMM_ID("COMM");
static const iff::fourcc SSND_ID("SSND");

// Compression types
static const iff::fourcc COMP_NONE("NONE");
static const iff::fourcc COMP_SOWT("sowt");
static const iff::fourcc COMP_FL32("fl32");
static const iff::fourcc COMP_FL64("fl64");
static const iff::fourcc COMP_ALAW("ALAW");
static const iff::fourcc COMP_ULAW("ULAW");
static const iff::fourcc COMP_alaw("alaw");
static const iff::fourcc COMP_ulaw("ulaw");
static const iff::fourcc COMP_IMA4("ima4");

aiff_container::aiff_container(io_stream* io)
    : m_io(io)
    , m_is_aifc(false)
    , m_current_frame(0)
    , m_audio_data_offset(0) {
    m_comm = {};
    m_ssnd = {};
    m_comm.compression_type = COMP_NONE;
}

aiff_container::~aiff_container() = default;

void aiff_container::parse() {
    if (!m_io) {
        throw std::runtime_error("No IO stream provided");
    }
    
    // Seek to beginning
    m_io->seek(0, seek_origin::set);
    
    // Parse FORM header
    parse_form_header();
    
    // Read all chunks
    while (m_io->tell() < m_io->get_size()) {
        chunk_info chunk;
        
        // Read chunk ID
        chunk.id = read_fourcc();
        if (chunk.id.to_uint32() == 0) {
            break;  // End of file or invalid chunk
        }
        
        // Read chunk size
        uint32_t size;
        if (m_io->read(&size, 4) != 4) {
            break;
        }
        chunk.size = iff::swap32be(size);
        chunk.offset = m_io->tell();
        
        // Store chunk info
        m_chunks[chunk.id] = chunk;
        
        // Parse specific chunks
        if (chunk.id == COMM_ID) {
            parse_comm_chunk(chunk);
        } else if (chunk.id == SSND_ID) {
            parse_ssnd_chunk(chunk);
        }
        
        // Skip to next chunk (account for padding)
        size_t next_offset = chunk.offset + chunk.size;
        if (chunk.size & 1) {
            next_offset++;  // Chunks are padded to even bytes
        }
        m_io->seek(next_offset, seek_origin::set);
    }
    
    // Validate we have required chunks
    if (m_comm.num_channels == 0) {
        throw std::runtime_error("Missing or invalid COMM chunk");
    }
    if (m_ssnd.data_size == 0) {
        throw std::runtime_error("Missing or invalid SSND chunk");
    }
}

void aiff_container::parse_form_header() {
    // Read FORM header
    iff::fourcc magic = read_fourcc();
    if (magic != FORM_ID) {
        throw std::runtime_error("Not an AIFF file - missing FORM header");
    }
    
    // Read file size
    uint32_t file_size;
    m_io->read(&file_size, 4);
    file_size = iff::swap32be(file_size);
    
    // Read form type
    iff::fourcc form_type = read_fourcc();
    
    if (form_type == AIFF_ID) {
        m_is_aifc = false;
    } else if (form_type == AIFC_ID) {
        m_is_aifc = true;
    } else {
        throw std::runtime_error("Unknown AIFF form type: " + form_type.to_string());
    }
}

void aiff_container::parse_comm_chunk(const chunk_info& chunk) {
    m_io->seek(chunk.offset, seek_origin::set);
    
    // Read COMM chunk data (must be packed to avoid padding)
    #pragma pack(push, 1)
    struct {
        uint16_t num_channels;
        uint32_t num_sample_frames;
        uint16_t sample_size;
        uint8_t sample_rate[10];
    } comm;
    #pragma pack(pop)
    
    if (m_io->read(&comm, sizeof(comm)) != sizeof(comm)) {
        throw std::runtime_error("Failed to read COMM chunk");
    }
    
    m_comm.num_channels = iff::swap16be(comm.num_channels);
    m_comm.num_sample_frames = iff::swap32be(comm.num_sample_frames);
    m_comm.sample_size = iff::swap16be(comm.sample_size);
    m_comm.sample_rate = convert_extended_to_double(comm.sample_rate);
    
    // Debug: Log what we read
    #if 0  // Disable debug output
    std::cerr << "COMM chunk: channels=" << m_comm.num_channels 
              << " frames=" << m_comm.num_sample_frames
              << " size=" << m_comm.sample_size
              << " rate=" << m_comm.sample_rate << "\n";
    std::cerr << "Extended bytes: ";
    for (int i = 0; i < 10; ++i) {
        std::cerr << std::hex << (int)comm.sample_rate[i] << " ";
    }
    std::cerr << std::dec << "\n";
    #endif
    
    // For AIFC, read compression type
    if (m_is_aifc && chunk.size > 18) {
        m_comm.compression_type = read_fourcc();
        
        #if 0  // Disable debug output
        std::cerr << "Compression type: " << m_comm.compression_type.to_string() 
                  << " (" << m_comm.compression_type[0] << m_comm.compression_type[1] 
                  << m_comm.compression_type[2] << m_comm.compression_type[3] << ")\n";
        #endif
        
        // Read compression name (Pascal string)
        uint8_t name_len;
        if (m_io->read(&name_len, 1) == 1 && name_len > 0) {
            std::vector<char> name(name_len + 1, 0);
            m_io->read(name.data(), name_len);
            m_comm.compression_name = name.data();
        }
    } else if (!m_is_aifc) {
        m_comm.compression_type = COMP_NONE;
    }
}

void aiff_container::parse_ssnd_chunk(const chunk_info& chunk) {
    m_io->seek(chunk.offset, seek_origin::set);
    
    // Read SSND header
    uint32_t offset, block_size;
    m_io->read(&offset, 4);
    m_io->read(&block_size, 4);
    
    m_ssnd.data_offset = iff::swap32be(offset);
    m_ssnd.block_size = iff::swap32be(block_size);
    m_ssnd.data_size = chunk.size - 8;  // Subtract header size
    
    // Calculate absolute offset to audio data
    m_audio_data_offset = chunk.offset + 8 + m_ssnd.data_offset;
}

iff::fourcc aiff_container::read_fourcc() {
    char id[4];
    if (m_io->read(id, 4) != 4) {
        return iff::fourcc(0);  // Invalid fourcc
    }
    // FourCC is stored as 4 ASCII characters in the file
    // Use from_bytes which is designed for 4-byte arrays
    return iff::fourcc::from_bytes(id);
}

double aiff_container::convert_extended_to_double(const uint8_t* extended) {
    // Convert 80-bit IEEE extended precision to double
    // This is a simplified version - full implementation would handle all cases
    
    uint16_t exponent = ((extended[0] & 0x7F) << 8) | extended[1];
    uint64_t mantissa = 0;
    for (int i = 0; i < 8; ++i) {
        mantissa = (mantissa << 8) | extended[2 + i];
    }
    
    // Check for common sample rates first
    if (exponent == 0x400E && mantissa == 0xAC44000000000000ULL) return 44100.0;
    if (exponent == 0x400E && mantissa == 0xBB80000000000000ULL) return 48000.0;
    if (exponent == 0x400D && mantissa == 0xFA00000000000000ULL) return 32000.0;
    if (exponent == 0x400C && mantissa == 0xBF40000000000000ULL) return 22050.0;
    if (exponent == 0x400C && mantissa == 0xAC44000000000000ULL) return 11025.0;
    if (exponent == 0x400C && mantissa == 0x9F40000000000000ULL) return 8000.0;
    
    // Handle special case of zero
    if (exponent == 0 && mantissa == 0) return 0.0;
    
    bool sign = extended[0] & 0x80;
    int exp = exponent - 16383;
    double result = static_cast<double>(mantissa) / (1ULL << 63);
    result = ldexp(result, exp);
    
    return sign ? -result : result;
}

codec_params aiff_container::get_codec_params() const {
    codec_params params;
    params.sample_rate = static_cast<uint32_t>(m_comm.sample_rate);
    params.channels = m_comm.num_channels;
    params.bits_per_sample = m_comm.sample_size;
    params.num_frames = m_comm.num_sample_frames;
    params.compression_type = m_comm.compression_type;
    
    // Special handling for IMA4
    if (m_comm.compression_type == COMP_IMA4) {
        params.frames_per_packet = 64;
        params.bytes_per_packet = 34;
    }
    
    return params;
}

size_t aiff_container::read_audio_data(uint8_t* buffer, size_t bytes_to_read) {
    if (!m_io || m_audio_data_offset == 0) {
        return 0;
    }
    
    #if 0
    std::cerr << "read_audio_data: m_current_frame=" << m_current_frame 
              << " bytes_to_read=" << bytes_to_read 
              << " data_size=" << m_ssnd.data_size << "\n";
    #endif
    
    // Calculate current byte position in audio data
    uint64_t current_byte = 0;
    
    // For compressed formats, we need to calculate byte position differently
    if (m_comm.compression_type == COMP_IMA4) {
        // IMA4: 34 bytes per 64 frames per channel
        uint64_t blocks = m_current_frame / 64;
        current_byte = blocks * 34 * m_comm.num_channels;
        #if 0
        std::cerr << "  IMA4: blocks=" << blocks << " current_byte=" << current_byte << "\n";
        #endif
    } else if (m_comm.compression_type == COMP_ULAW || m_comm.compression_type == COMP_ulaw ||
               m_comm.compression_type == COMP_ALAW || m_comm.compression_type == COMP_alaw) {
        // G.711 compression: 1 byte per sample in the actual data
        current_byte = m_current_frame * 1 * m_comm.num_channels;
    } else {
        // Uncompressed: direct calculation
        // Special case for 12-bit samples which are packed
        if (m_comm.sample_size == 12) {
            // 12-bit samples: 2 samples in 3 bytes
            // Calculate position based on packed format
            uint64_t sample_pairs = m_current_frame / 2;
            uint64_t odd_sample = m_current_frame % 2;
            current_byte = (sample_pairs * 3 + odd_sample * 2) * m_comm.num_channels;
        } else {
            size_t bytes_per_sample = (m_comm.sample_size + 7) / 8;
            current_byte = m_current_frame * bytes_per_sample * m_comm.num_channels;
        }
    }
    
    // Don't read past end of audio data
    uint64_t remaining = m_ssnd.data_size - current_byte;
    if (bytes_to_read > remaining) {
        bytes_to_read = remaining;
    }
    
    // Seek to position and read
    size_t abs_position = m_audio_data_offset + current_byte;
    m_io->seek(abs_position, seek_origin::set);
    
    #if 0
    // Debug all IMA4 reads
    if (m_comm.compression_type == COMP_IMA4) {
        std::cerr << "V3 Container read: frame=" << m_current_frame 
                  << " abs_pos=" << abs_position
                  << " bytes=" << bytes_to_read << "\n";
    }
    #endif
    
    size_t bytes_read = m_io->read(buffer, bytes_to_read);
    
    // Update position
    if (m_comm.compression_type == COMP_IMA4) {
        // IMA4: update by blocks
        size_t blocks_read = bytes_read / (34 * m_comm.num_channels);
        m_current_frame += blocks_read * 64;
    } else if (m_comm.compression_type == COMP_ULAW || m_comm.compression_type == COMP_ulaw ||
               m_comm.compression_type == COMP_ALAW || m_comm.compression_type == COMP_alaw) {
        // G.711: 1 byte per sample in actual data
        size_t frames_read = bytes_read / (1 * m_comm.num_channels);
        m_current_frame += frames_read;
    } else {
        // Uncompressed: update by samples
        if (m_comm.sample_size == 12) {
            // 12-bit samples: 2 samples in 3 bytes
            // Each 3 bytes represents 2 frames (for mono) or 2 frames worth of data (for stereo)
            size_t frames_read = (bytes_read * 2) / (3 * m_comm.num_channels);
            m_current_frame += frames_read;
        } else {
            size_t bytes_per_sample = (m_comm.sample_size + 7) / 8;
            size_t frames_read = bytes_read / (bytes_per_sample * m_comm.num_channels);
            m_current_frame += frames_read;
        }
    }
    
    return bytes_read;
}

bool aiff_container::seek_to_frame(uint64_t frame_position) {
    if (frame_position > m_comm.num_sample_frames) {
        return false;
    }
    
    m_current_frame = frame_position;
    return true;
}

const chunk_info* aiff_container::find_chunk(const iff::fourcc& chunk_id) const {
    auto it = m_chunks.find(chunk_id);
    if (it != m_chunks.end()) {
        return &it->second;
    }
    return nullptr;
}

std::vector<uint8_t> aiff_container::read_chunk(const iff::fourcc& chunk_id) {
    const chunk_info* chunk = find_chunk(chunk_id);
    if (!chunk) {
        return {};
    }
    
    std::vector<uint8_t> data(chunk->size);
    m_io->seek(chunk->offset, seek_origin::set);
    m_io->read(data.data(), chunk->size);
    
    return data;
}

} // namespace aiff
} // namespace musac