#include <musac/sdk/audio_converter.hh>
#include <musac/sdk/endian.hh>
#include <cstring>
#include <algorithm>

// Internal conversion functions
namespace musac::detail {
    void fast_swap16_inplace(uint8* data, size_t len);
    void fast_swap32_inplace(uint8* data, size_t len);
    buffer<uint8> fast_mono_to_stereo(const uint8* data, size_t len, audio_format format);
    buffer<uint8> fast_stereo_to_mono(const uint8* data, size_t len, audio_format format);
    buffer<uint8> convert_format(const uint8* data, size_t len, 
                                 audio_format from, audio_format to, uint8 channels);
    buffer<uint8> resample_cubic(const uint8* data, size_t len,
                                 audio_format format, uint8 channels,
                                 uint32 src_freq, uint32 dst_freq);
}

namespace musac {

// Implementation of audio_converter static methods

buffer<uint8> audio_converter::convert(const audio_spec& src_spec, 
                                       const uint8* src_data, 
                                       size_t src_len,
                                       const audio_spec& dst_spec) {
    // Handle empty input
    if (src_len == 0 || src_data == nullptr) {
        return buffer<uint8>(0);
    }
    
    // Check for fast paths
    
    // Fast path 1: No conversion needed
    if (!needs_conversion(src_spec, dst_spec)) {
        buffer<uint8> output(static_cast<unsigned int>(src_len));
        std::memcpy(output.data(), src_data, src_len);
        return output;
    }
    
    // Fast path 2: Endian swap only
    if (src_spec.channels == dst_spec.channels && src_spec.freq == dst_spec.freq) {
        if ((src_spec.format == audio_format::s16le && dst_spec.format == audio_format::s16be) ||
            (src_spec.format == audio_format::s16be && dst_spec.format == audio_format::s16le)) {
            buffer<uint8> output(static_cast<unsigned int>(src_len));
            std::memcpy(output.data(), src_data, src_len);
            detail::fast_swap16_inplace(output.data(), src_len);
            return output;
        }
        if ((src_spec.format == audio_format::s32le && dst_spec.format == audio_format::s32be) ||
            (src_spec.format == audio_format::s32be && dst_spec.format == audio_format::s32le) ||
            (src_spec.format == audio_format::f32le && dst_spec.format == audio_format::f32be) ||
            (src_spec.format == audio_format::f32be && dst_spec.format == audio_format::f32le)) {
            buffer<uint8> output(static_cast<unsigned int>(src_len));
            std::memcpy(output.data(), src_data, src_len);
            detail::fast_swap32_inplace(output.data(), src_len);
            return output;
        }
    }
    
    // Fast path 3: Channel conversion only (mono to stereo or stereo to mono)
    if (src_spec.format == dst_spec.format && src_spec.freq == dst_spec.freq) {
        if (src_spec.channels == 1 && dst_spec.channels == 2) {
            return detail::fast_mono_to_stereo(src_data, src_len, src_spec.format);
        }
        if (src_spec.channels == 2 && dst_spec.channels == 1) {
            return detail::fast_stereo_to_mono(src_data, src_len, src_spec.format);
        }
    }
    
    // Complex conversion path - may need multiple stages
    buffer<uint8> working_buffer(static_cast<unsigned int>(src_len));
    std::memcpy(working_buffer.data(), src_data, src_len);
    audio_spec working_spec = src_spec;
    
    // Stage 1: Format conversion
    if (src_spec.format != dst_spec.format) {
        working_buffer = detail::convert_format(working_buffer.data(), working_buffer.size(),
                                               working_spec.format, dst_spec.format, 
                                               working_spec.channels);
        if (working_buffer.size() == 0) {
            throw audio_conversion_error("Unsupported format conversion");
        }
        working_spec.format = dst_spec.format;
    }
    
    // Stage 2: Channel conversion
    if (working_spec.channels != dst_spec.channels) {
        if (working_spec.channels == 1 && dst_spec.channels == 2) {
            working_buffer = detail::fast_mono_to_stereo(working_buffer.data(), 
                                                         working_buffer.size(),
                                                         working_spec.format);
        } else if (working_spec.channels == 2 && dst_spec.channels == 1) {
            working_buffer = detail::fast_stereo_to_mono(working_buffer.data(),
                                                         working_buffer.size(),
                                                         working_spec.format);
        }
        working_spec.channels = dst_spec.channels;
    }
    
    // Stage 3: Sample rate conversion
    if (working_spec.freq != dst_spec.freq) {
        working_buffer = detail::resample_cubic(working_buffer.data(), working_buffer.size(),
                                               working_spec.format, working_spec.channels,
                                               working_spec.freq, dst_spec.freq);
        working_spec.freq = dst_spec.freq;
    }
    
    return working_buffer;
}

void audio_converter::convert_in_place(audio_spec& spec, 
                                       uint8* data, 
                                       size_t len,
                                       const audio_spec& dst_spec) {
    // Check if in-place conversion is possible
    bool can_convert_inplace = false;
    
    // Same format but different endianness
    if (spec.channels == dst_spec.channels && spec.freq == dst_spec.freq) {
        if ((spec.format == audio_format::s16le && dst_spec.format == audio_format::s16be) ||
            (spec.format == audio_format::s16be && dst_spec.format == audio_format::s16le)) {
            // Swap bytes for 16-bit samples
            int16* samples = reinterpret_cast<int16*>(data);
            size_t num_samples = len / sizeof(int16);
            for (size_t i = 0; i < num_samples; ++i) {
                samples[i] = swap16be(samples[i]);
            }
            spec.format = dst_spec.format;
            can_convert_inplace = true;
        } else if ((spec.format == audio_format::s32le && dst_spec.format == audio_format::s32be) ||
                   (spec.format == audio_format::s32be && dst_spec.format == audio_format::s32le)) {
            // Swap bytes for 32-bit samples
            int32* samples = reinterpret_cast<int32*>(data);
            size_t num_samples = len / sizeof(int32);
            for (size_t i = 0; i < num_samples; ++i) {
                samples[i] = swap32be(samples[i]);
            }
            spec.format = dst_spec.format;
            can_convert_inplace = true;
        }
    }
    
    if (!can_convert_inplace) {
        throw audio_conversion_error("In-place conversion not possible for this format combination");
    }
}

bool audio_converter::has_fast_path(const audio_spec& from, const audio_spec& to) {
    // Fast endian swap
    if (from.channels == to.channels && from.freq == to.freq) {
        if ((from.format == audio_format::s16le && to.format == audio_format::s16be) ||
            (from.format == audio_format::s16be && to.format == audio_format::s16le) ||
            (from.format == audio_format::s32le && to.format == audio_format::s32be) ||
            (from.format == audio_format::s32be && to.format == audio_format::s32le)) {
            return true;
        }
    }
    
    // Fast mono to stereo duplication
    if (from.channels == 1 && to.channels == 2 && 
        from.freq == to.freq && from.format == to.format) {
        return true;
    }
    
    // Fast stereo to mono averaging
    if (from.channels == 2 && to.channels == 1 && 
        from.freq == to.freq && from.format == to.format) {
        return true;
    }
    
    return false;
}

size_t audio_converter::estimate_output_size(const audio_spec& src_spec,
                                            size_t src_len,
                                            const audio_spec& dst_spec) {
    // Validate formats
    if (static_cast<uint16>(src_spec.format) > 0xFF00) {
        throw unsupported_format_error(src_spec.format);
    }
    if (static_cast<uint16>(dst_spec.format) > 0xFF00) {
        throw unsupported_format_error(dst_spec.format);
    }
    
    // Calculate number of samples in source
    size_t bytes_per_sample_src = audio_format_byte_size(src_spec.format) * src_spec.channels;
    if (bytes_per_sample_src == 0) {
        throw unsupported_format_error(src_spec.format);
    }
    
    size_t num_frames = src_len / bytes_per_sample_src;
    
    // Adjust for sample rate conversion
    if (src_spec.freq != dst_spec.freq && src_spec.freq > 0) {
        num_frames = (num_frames * dst_spec.freq) / src_spec.freq;
        // Add a small buffer for rounding and interpolation
        num_frames += 4;
    }
    
    // Calculate output size
    size_t bytes_per_sample_dst = audio_format_byte_size(dst_spec.format) * dst_spec.channels;
    if (bytes_per_sample_dst == 0) {
        throw unsupported_format_error(dst_spec.format);
    }
    
    return num_frames * bytes_per_sample_dst;
}

size_t audio_converter::convert_into(const audio_spec& src_spec,
                                     const uint8* src_data,
                                     size_t src_len,
                                     const audio_spec& dst_spec,
                                     buffer<uint8>& dst_buffer) {
    // Use the main convert function
    buffer<uint8> result = convert(src_spec, src_data, src_len, dst_spec);
    
    // Ensure destination buffer is large enough
    if (dst_buffer.size() < result.size()) {
        dst_buffer.resize(result.size());
    }
    
    // Copy to provided buffer
    std::memcpy(dst_buffer.data(), result.data(), result.size());
    
    return result.size();
}

// Stream converter implementation
struct audio_converter::stream_converter::impl {
    audio_spec from_spec;
    audio_spec to_spec;
    
    // Buffers for streaming
    buffer<uint8> input_buffer;      // Accumulated input data
    buffer<uint8> output_buffer;     // Converted output ready to return
    size_t input_accumulated;         // Bytes accumulated in input_buffer
    size_t output_available;          // Bytes available in output_buffer
    size_t output_consumed;           // Bytes already consumed from output_buffer
    
    // For resampling state
    float last_sample_value[8];      // Last sample values per channel (max 8 channels)
    double sample_position;           // Fractional sample position for resampling
    
    impl(const audio_spec& from, const audio_spec& to) 
        : from_spec(from), to_spec(to),
          input_buffer(16384),        // 16KB input buffer
          output_buffer(16384),       // 16KB output buffer
          input_accumulated(0),
          output_available(0),
          output_consumed(0),
          sample_position(0.0) {
        // Initialize last samples to silence
        for (size_t i = 0; i < 8; ++i) {
            last_sample_value[i] = 0.0f;
        }
    }
    
    size_t get_min_input_size() const {
        // Minimum input size needed for one output frame
        size_t input_frame_size = audio_format_byte_size(from_spec.format) * from_spec.channels;
        size_t output_frame_size = audio_format_byte_size(to_spec.format) * to_spec.channels;
        
        // For resampling, we need at least 4 input frames for cubic interpolation
        if (from_spec.freq != to_spec.freq) {
            return input_frame_size * 4;
        }
        return input_frame_size;
    }
};

audio_converter::stream_converter::stream_converter(const audio_spec& from, const audio_spec& to)
    : m_pimpl(std::make_unique<impl>(from, to)) {
}

audio_converter::stream_converter::~stream_converter() = default;

size_t audio_converter::stream_converter::process_chunk(const uint8* input, size_t input_len,
                                                       buffer<uint8>& output) {
    // Return any previously converted data first
    if (m_pimpl->output_available > m_pimpl->output_consumed) {
        size_t available = m_pimpl->output_available - m_pimpl->output_consumed;
        size_t to_copy = std::min(available, output.size());
        std::memcpy(output.data(), 
                   m_pimpl->output_buffer.data() + m_pimpl->output_consumed, 
                   to_copy);
        m_pimpl->output_consumed += to_copy;
        
        // Reset if all consumed
        if (m_pimpl->output_consumed >= m_pimpl->output_available) {
            m_pimpl->output_available = 0;
            m_pimpl->output_consumed = 0;
        }
        return to_copy;
    }
    
    // Add new input to accumulated buffer
    if (input_len > 0) {
        // Ensure buffer is large enough
        if (m_pimpl->input_accumulated + input_len > m_pimpl->input_buffer.size()) {
            m_pimpl->input_buffer.resize(static_cast<unsigned int>(m_pimpl->input_accumulated + input_len + 4096));
        }
        std::memcpy(m_pimpl->input_buffer.data() + m_pimpl->input_accumulated, 
                   input, input_len);
        m_pimpl->input_accumulated += input_len;
    }
    
    // Check if we have enough input to process
    size_t min_input = m_pimpl->get_min_input_size();
    if (m_pimpl->input_accumulated < min_input) {
        return 0; // Need more input
    }
    
    // Calculate how much we can process (align to frame boundaries)
    size_t input_frame_size = audio_format_byte_size(m_pimpl->from_spec.format) * 
                             m_pimpl->from_spec.channels;
    size_t frames_available = m_pimpl->input_accumulated / input_frame_size;
    
    // For resampling, keep some frames for interpolation
    if (m_pimpl->from_spec.freq != m_pimpl->to_spec.freq && frames_available > 4) {
        frames_available -= 3; // Keep 3 frames for cubic interpolation
    }
    
    size_t bytes_to_process = frames_available * input_frame_size;
    
    // Convert the accumulated input
    try {
        buffer<uint8> converted = audio_converter::convert(
            m_pimpl->from_spec,
            m_pimpl->input_buffer.data(),
            bytes_to_process,
            m_pimpl->to_spec);
        
        // Store converted data
        if (converted.size() > m_pimpl->output_buffer.size()) {
            m_pimpl->output_buffer.resize(converted.size());
        }
        std::memcpy(m_pimpl->output_buffer.data(), converted.data(), converted.size());
        m_pimpl->output_available = converted.size();
        m_pimpl->output_consumed = 0;
        
        // Move remaining input to beginning of buffer
        size_t remaining = m_pimpl->input_accumulated - bytes_to_process;
        if (remaining > 0) {
            std::memmove(m_pimpl->input_buffer.data(),
                        m_pimpl->input_buffer.data() + bytes_to_process,
                        remaining);
        }
        m_pimpl->input_accumulated = remaining;
        
        // Return what fits in output buffer
        size_t to_return = std::min(converted.size(), output.size());
        std::memcpy(output.data(), m_pimpl->output_buffer.data(), to_return);
        m_pimpl->output_consumed = to_return;
        
        return to_return;
        
    } catch (const audio_conversion_error&) {
        // Conversion failed, return 0
        return 0;
    }
}

size_t audio_converter::stream_converter::flush(buffer<uint8>& output) {
    // First, return any remaining converted data
    if (m_pimpl->output_available > m_pimpl->output_consumed) {
        size_t available = m_pimpl->output_available - m_pimpl->output_consumed;
        size_t to_copy = std::min(available, output.size());
        std::memcpy(output.data(), 
                   m_pimpl->output_buffer.data() + m_pimpl->output_consumed, 
                   to_copy);
        m_pimpl->output_consumed += to_copy;
        return to_copy;
    }
    
    // Process any remaining input (even if less than minimum)
    if (m_pimpl->input_accumulated > 0) {
        try {
            buffer<uint8> converted = audio_converter::convert(
                m_pimpl->from_spec,
                m_pimpl->input_buffer.data(),
                m_pimpl->input_accumulated,
                m_pimpl->to_spec);
            
            m_pimpl->input_accumulated = 0;
            
            size_t to_return = std::min(converted.size(), output.size());
            std::memcpy(output.data(), converted.data(), to_return);
            
            // Store any remainder
            if (converted.size() > to_return) {
                if (converted.size() > m_pimpl->output_buffer.size()) {
                    m_pimpl->output_buffer.resize(converted.size());
                }
                std::memcpy(m_pimpl->output_buffer.data(), 
                           converted.data() + to_return,
                           converted.size() - to_return);
                m_pimpl->output_available = converted.size() - to_return;
                m_pimpl->output_consumed = 0;
            }
            
            return to_return;
        } catch (const audio_conversion_error&) {
            // Conversion failed
            m_pimpl->input_accumulated = 0;
            return 0;
        }
    }
    
    return 0;
}

void audio_converter::stream_converter::reset() {
    // Clear all buffers and state
    m_pimpl->input_accumulated = 0;
    m_pimpl->output_available = 0;
    m_pimpl->output_consumed = 0;
    m_pimpl->sample_position = 0.0;
    
    // Reset last sample values to silence
    for (size_t i = 0; i < 8; ++i) {
        m_pimpl->last_sample_value[i] = 0.0f;
    }
    
    // Clear buffers (optional, but ensures no stale data)
    std::fill_n(m_pimpl->input_buffer.data(), m_pimpl->input_buffer.size(), 0);
    std::fill_n(m_pimpl->output_buffer.data(), m_pimpl->output_buffer.size(), 0);
}

} // namespace musac