#ifndef MUSAC_SDL3_AUDIO_BACKEND_HH
#define MUSAC_SDL3_AUDIO_BACKEND_HH

#include <musac/audio_backend.hh>

namespace musac {

class sdl3_audio_backend : public audio_backend {
public:
    sdl3_audio_backend();
    ~sdl3_audio_backend() override;
    
    bool init() override;
    void shutdown() override;
    std::string get_name() const override;
    bool is_initialized() const override;
    
private:
    bool m_initialized;
};

} // namespace musac

#endif // MUSAC_SDL3_AUDIO_BACKEND_HH