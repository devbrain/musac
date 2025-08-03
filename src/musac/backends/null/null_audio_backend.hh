#ifndef MUSAC_NULL_AUDIO_BACKEND_HH
#define MUSAC_NULL_AUDIO_BACKEND_HH

#include <musac/audio_backend.hh>

namespace musac {

/**
 * Null audio backend for testing and headless environments.
 * This backend does nothing and always reports success.
 */
class null_audio_backend : public audio_backend {
public:
    null_audio_backend() = default;
    ~null_audio_backend() override = default;
    
    void init() override { /* No-op for null backend */ }
    void shutdown() override {}
    std::string get_name() const override { return "Null"; }
    bool is_initialized() const override { return true; }
};

} // namespace musac

#endif // MUSAC_NULL_AUDIO_BACKEND_HH