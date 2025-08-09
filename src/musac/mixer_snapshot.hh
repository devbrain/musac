#ifndef MUSAC_MIXER_SNAPSHOT_HH
#define MUSAC_MIXER_SNAPSHOT_HH

#include <vector>
#include <chrono>
#include <cstdint>
#include <musac/sdk/types.hh>

namespace musac {

// Forward declarations
class fade_envelope;

/**
 * Captures the complete state of the audio mixer for device switching.
 * This allows seamless audio continuity when switching between devices.
 */
struct mixer_snapshot {
    // Audio buffer with mixed samples ready to play
    std::vector<float> pending_samples;
    size_t buffer_position = 0;
    
    // Individual stream state
    struct stream_state {
        int token;
        uint64_t playback_tick;          // Current playback position in ticks
        size_t current_frame;            // Frame position within the audio source
        float volume;
        float internal_volume;
        float stereo_pos;
        bool is_playing;
        bool is_paused;
        bool is_muted;
        bool starting;
        size_t current_iteration;        // For looping
        size_t wanted_iterations;
        
        // Volume and fade state
        float fade_gain;
        int fade_state;                  // FadeEnvelope::State
        
        // Timing information
        uint64_t playback_start_tick;
    };
    
    // All active stream states
    std::vector<stream_state> active_streams;
    
    // Global timing information
    std::chrono::steady_clock::time_point snapshot_time;
    uint64_t global_tick_count;         // SDL tick count at snapshot
    
    // Audio format at time of snapshot
    struct {
        channels_t channels;
        sample_rate_t freq;
        int format;  // audio_format enum value
    } audio_spec;
};

} // namespace musac

#endif // MUSAC_MIXER_SNAPSHOT_HH