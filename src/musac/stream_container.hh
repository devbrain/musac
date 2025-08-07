//
// Stream container with proper lifetime management and thread safety
//

#pragma once

#include <vector>
#include <memory>
#include <shared_mutex>
#include <mutex>
#include <algorithm>
#include <functional>
#include <atomic>

namespace musac {
    class audio_stream;
    
    // Thread-safe container for audio streams with lifetime management
    class stream_container {
    public:
        // Entry that tracks stream lifetime
        struct stream_entry {
            audio_stream* stream;
            std::weak_ptr<void> lifetime_token;
            int token_id;
            
            stream_entry(audio_stream* s, std::weak_ptr<void> token, int id) 
                : stream(s), lifetime_token(token), token_id(id) {}
            
            bool is_valid() const {
                return stream && !lifetime_token.expired();
            }
        };
        
        stream_container() = default;
        ~stream_container() = default;
        
        // Container is not copyable (contains raw pointers)
        stream_container(const stream_container&) = delete;
        stream_container& operator=(const stream_container&) = delete;
        
        // Move operations are safe with mutex and atomic
        stream_container(stream_container&&) = default;
        stream_container& operator=(stream_container&&) = default;
        
        // Add a stream and its lifetime token
        void add(audio_stream* stream, std::weak_ptr<void> lifetime_token, int token_id) {
            if (!stream) return;
            
            std::unique_lock lock(m_mutex);
            
            // Check if stream already exists
            auto it = std::find_if(m_entries.begin(), m_entries.end(),
                [token_id](const stream_entry& e) { return e.token_id == token_id; });
                
            if (it != m_entries.end()) {
                // Update existing entry
                it->lifetime_token = lifetime_token;
            } else {
                // Add new entry
                m_entries.emplace_back(stream, lifetime_token, token_id);
            }
            
            // Periodic cleanup
            maybe_cleanup();
        }
        
        // Remove a stream by token ID
        void remove(int token_id) {
            std::unique_lock lock(m_mutex);
            
            auto it = std::remove_if(m_entries.begin(), m_entries.end(),
                [token_id](const stream_entry& e) { 
                    return e.token_id == token_id; 
                });
                
            m_entries.erase(it, m_entries.end());
        }
        
        // Update stream pointer when stream is moved
        void update_stream_pointer(int token_id, audio_stream* new_stream) {
            std::unique_lock lock(m_mutex);
            
            auto it = std::find_if(m_entries.begin(), m_entries.end(),
                [token_id](const stream_entry& e) { return e.token_id == token_id; });
                
            if (it != m_entries.end()) {
                it->stream = new_stream;
            }
        }
        
        // Get a snapshot of valid streams for the audio callback
        std::shared_ptr<std::vector<stream_entry>> get_valid_streams() const {
            std::shared_lock lock(m_mutex);
            
            auto result = std::make_shared<std::vector<stream_entry>>();
            result->reserve(m_entries.size());
            
            // Copy only valid entries
            for (const auto& entry : m_entries) {
                if (entry.is_valid()) {
                    result->push_back(entry);
                }
            }
            
            return result;
        }
        
        // Process all valid streams
        template<typename Func>
        void for_each_valid(Func&& func) {
            auto valid_streams = get_valid_streams();
            
            for (const auto& entry : *valid_streams) {
                if (entry.is_valid()) {
                    func(entry.stream);
                }
            }
        }
        
        // Get count of valid streams
        size_t valid_count() const {
            std::shared_lock lock(m_mutex);
            return static_cast<size_t>(std::count_if(m_entries.begin(), m_entries.end(),
                [](const stream_entry& e) { return e.is_valid(); }));
        }
        
        // Force cleanup of invalid entries
        void cleanup() {
            std::unique_lock lock(m_mutex);
            cleanup_invalid_entries();
        }
        
    private:
        mutable std::shared_mutex m_mutex;
        std::vector<stream_entry> m_entries;
        mutable std::atomic<int> m_cleanup_counter{0};
        
        // Remove invalid entries
        void cleanup_invalid_entries() {
            auto it = std::remove_if(m_entries.begin(), m_entries.end(),
                [](const stream_entry& e) { return !e.is_valid(); });
                
            m_entries.erase(it, m_entries.end());
        }
        
        // Periodic cleanup to prevent memory bloat
        void maybe_cleanup() {
            // Cleanup every 100 operations
            if (++m_cleanup_counter % 100 == 0) {
                cleanup_invalid_entries();
            }
        }
    };
}