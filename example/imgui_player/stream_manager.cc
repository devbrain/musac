#include "stream_manager.hh"
#include "audio_device_manager.hh"
#include <musac/audio_source.hh>
#include <musac/test_data/loader.hh>
#include <algorithm>

namespace musac::player {

StreamManager::StreamManager(AudioDeviceManager* device_manager)
    : m_device_manager(device_manager) {
}

StreamManager::~StreamManager() {
    stop_all_streams();
}

bool StreamManager::play_music(test_data::music_type type) {
    if (!m_device_manager || !m_device_manager->is_device_open()) {
        return false;
    }
    
    // Stop current music if playing
    stop_music();
    
    try {
        // Create new music stream
        m_music_stream = create_stream_from_type(type);
        if (!m_music_stream) {
            return false;
        }
        
        // Set volume and play
        m_music_stream->set_volume(m_music_volume);
        m_music_stream->play();
        return true;
        
    } catch (const std::exception& e) {
        m_music_stream.reset();
        return false;
    }
}

void StreamManager::stop_music() {
    if (m_music_stream) {
        m_music_stream->stop();
        m_music_stream.reset();
    }
}

bool StreamManager::is_music_playing() const {
    return m_music_stream && m_music_stream->is_playing();
}

void StreamManager::set_music_volume(float volume) {
    m_music_volume = std::clamp(volume, 0.0f, 1.0f);
    if (m_music_stream) {
        m_music_stream->set_volume(m_music_volume);
    }
}

bool StreamManager::play_sound(test_data::music_type type) {
    if (!m_device_manager || !m_device_manager->is_device_open()) {
        return false;
    }
    
    try {
        // Clean up finished sounds first
        cleanup_finished_streams();
        
        // Create new sound stream
        auto sound_stream = create_stream_from_type(type);
        if (!sound_stream) {
            return false;
        }
        
        // Set volume and play
        sound_stream->set_volume(m_sound_volume);
        sound_stream->play();
        
        // Add to active sounds
        m_sound_streams.push_back(std::move(sound_stream));
        return true;
        
    } catch (const std::exception& e) {
        return false;
    }
}

void StreamManager::stop_all_sounds() {
    for (auto& stream : m_sound_streams) {
        if (stream) {
            stream->stop();
        }
    }
    m_sound_streams.clear();
}

void StreamManager::set_sound_volume(float volume) {
    m_sound_volume = std::clamp(volume, 0.0f, 1.0f);
    for (auto& stream : m_sound_streams) {
        if (stream) {
            stream->set_volume(m_sound_volume);
        }
    }
}

void StreamManager::cleanup_finished_streams() {
    remove_finished_sounds();
}

void StreamManager::stop_all_streams() {
    stop_music();
    stop_all_sounds();
}

size_t StreamManager::get_total_active_streams() const {
    size_t count = 0;
    if (is_music_playing()) count++;
    count += m_sound_streams.size();
    return count;
}

bool StreamManager::has_active_streams() const {
    return is_music_playing() || !m_sound_streams.empty();
}

std::unique_ptr<audio_stream> StreamManager::create_stream_from_type(
    test_data::music_type type) {
    
    auto& device = m_device_manager->get_current_device();
    
    // Load audio source
    auto source = test_data::loader::load(type);
    
    // Create and open stream
    auto stream = std::make_unique<audio_stream>(device.create_stream(std::move(source)));
    stream->open();
    
    return stream;
}

void StreamManager::remove_finished_sounds() {
    m_sound_streams.erase(
        std::remove_if(m_sound_streams.begin(), m_sound_streams.end(),
            [](const std::unique_ptr<audio_stream>& stream) {
                return !stream || !stream->is_playing();
            }),
        m_sound_streams.end()
    );
}

} // namespace musac::player