#pragma once

#include <memory>
#include <vector>
#include <musac/stream.hh>

#include <musac/test_data/loader.hh>

namespace musac::player {

// Forward declaration
class AudioDeviceManager;

/**
 * StreamManager - Manages audio stream lifecycle and playback
 * 
 * Responsibilities:
 * - Create and manage music streams (single instance)
 * - Create and manage sound effect streams (multiple instances)
 * - Handle volume control
 * - Clean up finished streams
 * - Provide playback status
 */
class StreamManager {
public:
    explicit StreamManager(AudioDeviceManager* device_manager);
    ~StreamManager();

    // Music playback (single stream)
    bool play_music(test_data::music_type type);
    void stop_music();
    bool is_music_playing() const;
    void set_music_volume(float volume);
    float get_music_volume() const { return m_music_volume; }
    audio_stream* get_music_stream() { return m_music_stream.get(); }
    
    // Sound effects playback (multiple streams)
    bool play_sound(test_data::music_type type);
    void stop_all_sounds();
    size_t get_active_sound_count() const { return m_sound_streams.size(); }
    void set_sound_volume(float volume);
    float get_sound_volume() const { return m_sound_volume; }
    
    // Stream management
    void cleanup_finished_streams();
    void stop_all_streams();
    
    // Status
    size_t get_total_active_streams() const;
    bool has_active_streams() const;

private:
    AudioDeviceManager* m_device_manager;
    
    // Music (single stream)
    std::unique_ptr<audio_stream> m_music_stream;
    float m_music_volume = 0.5f;
    
    // Sound effects (multiple streams)
    std::vector<std::unique_ptr<audio_stream>> m_sound_streams;
    float m_sound_volume = 0.7f;
    
    // Helper methods
    std::unique_ptr<audio_stream> create_stream_from_type(test_data::music_type type);
    void remove_finished_sounds();
};

} // namespace musac::player