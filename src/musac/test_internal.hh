// Internal testing header - only included by test files
#pragma once

#include "audio_mixer.hh"
#include "stream.cc"  // Access to impl

namespace musac::test {

// Test accessor class - provides controlled access to internals for testing
class internal_test_access {
public:
    // Mixer testing
    static audio_mixer& get_stream_mixer() {
        return audio_stream::impl::s_mixer;
    }
    
    // Stream internals
    static audio_stream::impl* get_stream_impl(audio_stream* stream) {
        return stream->m_pimpl.get();
    }
};

} // namespace musac::test