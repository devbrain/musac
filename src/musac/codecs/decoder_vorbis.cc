//
// Created by igor on 3/25/25.
//

#include <musac/codecs/decoder_vorbis.hh>
#include <musac/sdk/io_stream.hh>
#include <musac/error.hh>
#include <failsafe/failsafe.hh>
#include <vector>
#include <cstring>

// Include stb_vorbis implementation
#define STB_VORBIS_HEADER_ONLY
#include "vorbis/stb_vorbis.c"

namespace musac {
    struct decoder_vorbis::impl {
        stb_vorbis* m_vorbis = nullptr;
        std::vector<uint8_t> m_data;  // Store the entire file data
        int m_channels = 0;
        int m_sample_rate = 0;
        uint32_t m_total_samples = 0;
        
        ~impl() {
            if (m_vorbis) {
                stb_vorbis_close(m_vorbis);
                m_vorbis = nullptr;
            }
        }
        
        void load_from_stream(io_stream* rwops) {
            // Get file size
            auto current_pos = rwops->tell();
            rwops->seek(0, seek_origin::end);
            auto file_size = rwops->tell();
            rwops->seek(current_pos, seek_origin::set);
            
            if (file_size <= 0) {
                THROW_RUNTIME("Invalid file size for Vorbis file");
            }
            
            // Read entire file into memory
            m_data.resize(file_size);
            auto bytes_read = rwops->read(m_data.data(), file_size);
            if (bytes_read != file_size) {
                THROW_RUNTIME("Failed to read complete Vorbis file");
            }
            
            // Open vorbis from memory
            int error = 0;
            m_vorbis = stb_vorbis_open_memory(m_data.data(), static_cast<int>(m_data.size()), 
                                              &error, nullptr);
            
            if (!m_vorbis) {
                // stb_vorbis doesn't provide comprehensive error codes in enum
                // Just provide a generic error message with the error code
                THROW_RUNTIME("Failed to open Vorbis file, error code: " + std::to_string(error));
            }
            
            // Get stream info
            stb_vorbis_info info = stb_vorbis_get_info(m_vorbis);
            m_channels = info.channels;
            m_sample_rate = info.sample_rate;
            
            // Get total samples
            m_total_samples = stb_vorbis_stream_length_in_samples(m_vorbis);
        }
    };
    
    decoder_vorbis::decoder_vorbis() 
        : m_pimpl(std::make_unique<impl>()) {
    }
    
    decoder_vorbis::~decoder_vorbis() = default;
    
    bool decoder_vorbis::do_accept(io_stream* rwops) {
        // stb_vorbis doesn't have a simple "test" function, so we need to check the header
        // Check for OggS magic
        char magic[4];
        if (rwops->read(magic, 4) != 4) {
            return false;
        }
        
        // Check for Ogg Vorbis signature
        return (magic[0] == 'O' && magic[1] == 'g' && 
                magic[2] == 'g' && magic[3] == 'S');
    }
    
    const char* decoder_vorbis::get_name() const {
        return "Vorbis";
    }
    
    void decoder_vorbis::open(io_stream* rwops) {
        m_pimpl->load_from_stream(rwops);
        set_is_open(true);
    }
    
    channels_t decoder_vorbis::get_channels() const {
        return static_cast<channels_t>(m_pimpl->m_channels);
    }
    
    sample_rate_t decoder_vorbis::get_rate() const {
        return static_cast<sample_rate_t>(m_pimpl->m_sample_rate);
    }
    
    bool decoder_vorbis::rewind() {
        if (!is_open() || !m_pimpl->m_vorbis) {
            return false;
        }
        
        // Seek to beginning
        return stb_vorbis_seek_start(m_pimpl->m_vorbis) != 0;
    }
    
    std::chrono::microseconds decoder_vorbis::duration() const {
        if (!is_open() || m_pimpl->m_total_samples == 0) {
            return std::chrono::microseconds(0);
        }
        
        // Calculate duration from total samples and sample rate
        double duration_seconds = static_cast<double>(m_pimpl->m_total_samples) / 
                                 static_cast<double>(m_pimpl->m_sample_rate);
        
        return std::chrono::microseconds(
            static_cast<int64_t>(duration_seconds * 1'000'000)
        );
    }
    
    bool decoder_vorbis::seek_to_time(std::chrono::microseconds pos) {
        if (!is_open() || !m_pimpl->m_vorbis) {
            return false;
        }
        
        // Convert time to sample position
        double seconds = pos.count() / 1'000'000.0;
        unsigned int target_sample = static_cast<unsigned int>(
            seconds * static_cast<double>(m_pimpl->m_sample_rate)
        );
        
        // Clamp to valid range
        if (target_sample >= m_pimpl->m_total_samples) {
            target_sample = m_pimpl->m_total_samples - 1;
        }
        
        // Seek to target position
        return stb_vorbis_seek(m_pimpl->m_vorbis, target_sample) != 0;
    }
    
    size_t decoder_vorbis::do_decode(float buf[], size_t len, bool& callAgain) {
        if (!is_open() || !m_pimpl->m_vorbis) {
            callAgain = false;
            return 0;
        }
        
        // stb_vorbis_get_samples_float_interleaved returns number of samples per channel
        // len is total number of floats in the buffer
        // For interleaved data: num_floats = num_samples_per_channel * num_channels
        int samples_per_channel_requested = static_cast<int>(len / m_pimpl->m_channels);
        
        int samples_decoded = stb_vorbis_get_samples_float_interleaved(
            m_pimpl->m_vorbis,
            m_pimpl->m_channels,
            buf,
            static_cast<int>(len)
        );
        
        // samples_decoded is the number of samples per channel actually decoded
        // Return total number of floats decoded
        size_t total_floats = samples_decoded * m_pimpl->m_channels;
        
        // Check if we should call again
        callAgain = (samples_decoded > 0 && samples_decoded == samples_per_channel_requested);
        
        return total_floats;
    }
}