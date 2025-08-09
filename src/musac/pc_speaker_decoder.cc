#include "pc_speaker_decoder.hh"
#include <cmath>
#include <algorithm>
#include <iostream>

namespace musac {
    
    pc_speaker_decoder::pc_speaker_decoder(pc_speaker_stream* parent)
        : m_parent(parent)
        , m_sample_rate(44100)
        , m_phase(0.0f)
        , m_phase_increment(0.0f)
        , m_current_frequency(0.0f)
        , m_current_tone{0.0f, 0, false} {
    }
    
    pc_speaker_decoder::~pc_speaker_decoder() = default;
    
    void pc_speaker_decoder::open(io_stream* stream) {
        // We don't use the stream for PC speaker
        (void)stream;
        set_is_open(true);
        m_phase = 0.0f;
        m_current_tone = {0.0f, 0, false};
    }
    
    sample_rate_t pc_speaker_decoder::get_rate() const {
        return m_sample_rate;
    }
    
    channels_t pc_speaker_decoder::get_channels() const {
        return 1;  // PC speaker is mono
    }
    
    size_t pc_speaker_decoder::do_decode(float buf[], size_t len, bool& call_again) {
        size_t samples_written = 0;
        
        while (samples_written < len) {
            // Check if we need a new tone
            if (!m_current_tone.active || m_current_tone.samples_remaining == 0) {
                if (!dequeue_next_tone()) {
                    // No more tones, fill rest with silence
                    for (size_t i = samples_written; i < len; i++) {
                        buf[i] = 0.0f;
                    }
                    // Keep the stream alive so we can add more tones
                    call_again = true;
                    return len;
                }
            }
            
            // Generate samples for current tone
            size_t samples_to_generate = std::min(len - samples_written, 
                                                  m_current_tone.samples_remaining);
            
            for (size_t i = 0; i < samples_to_generate; i++) {
                float sample = generate_sample();
                buf[samples_written + i] = sample;
                m_current_tone.samples_remaining--;
            }
            
            samples_written += samples_to_generate;
        }
        
        // Check if we have more tones queued
        call_again = !m_parent->is_queue_empty() || m_current_tone.active;
        return len;
    }
    
    bool pc_speaker_decoder::rewind() {
        m_phase = 0.0f;
        m_current_tone = {0.0f, 0, false};
        return true;
    }
    
    std::chrono::microseconds pc_speaker_decoder::duration() const {
        // PC speaker streams don't have a fixed duration
        return std::chrono::microseconds(0);
    }
    
    bool pc_speaker_decoder::seek_to_time(std::chrono::microseconds pos) {
        // Seeking not supported for PC speaker
        (void)pos;
        return false;
    }
    
    void pc_speaker_decoder::set_frequency(float hz) {
        if (hz != m_current_frequency) {
            m_current_frequency = hz;
            if (hz > 0.0f) {
                // Calculate phase increment for square wave
                m_phase_increment = (2.0f * hz) / static_cast<float>(m_sample_rate);
            } else {
                m_phase_increment = 0.0f;  // Silence
            }
        }
    }
    
    float pc_speaker_decoder::generate_sample() {
        if (m_current_frequency <= 0.0f) {
            return 0.0f;  // Silence
        }
        
        // Generate square wave
        m_phase += m_phase_increment;
        if (m_phase >= 1.0f) {
            m_phase -= 2.0f;
        }
        
        // Square wave: positive half = 0.3, negative half = -0.3
        // Scale down to prevent clipping
        return (m_phase >= 0.0f) ? 0.3f : -0.3f;
    }
    
    bool pc_speaker_decoder::dequeue_next_tone() {
        if (!m_parent) {
            return false;
        }
        
        std::lock_guard<std::mutex> lock(m_parent->m_queue_mutex);
        
        if (m_parent->m_tone_queue.empty()) {
            m_current_tone.active = false;
            return false;
        }
        
        // Get next tone from queue
        auto& tone = m_parent->m_tone_queue.front();
        
        // Calculate number of samples for this tone
        size_t samples = static_cast<size_t>(
            (tone.duration.count() * m_sample_rate) / 1000
        );
        
        // Set up current tone
        m_current_tone.frequency_hz = tone.frequency_hz;
        m_current_tone.samples_remaining = samples;
        m_current_tone.active = true;
        
        // Configure generator
        set_frequency(tone.frequency_hz);
        
        // Remove from queue
        m_parent->m_tone_queue.pop_front();
        
        return true;
    }
    
} // namespace musac