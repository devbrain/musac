//
// Created by igor on 4/28/25.
//

#include "musac/fade_envelop.hh"
#include <cmath>

namespace musac {

    void FadeEnvelope::startFadeIn(std::chrono::milliseconds duration) noexcept {
        m_duration = duration;
        m_startTime = std::chrono::steady_clock::now();
        m_state = State::FadeIn;
    }

    void FadeEnvelope::startFadeOut(std::chrono::milliseconds duration) noexcept {
        m_duration = duration;
        m_startTime = std::chrono::steady_clock::now();
        m_state = State::FadeOut;
    }

    float FadeEnvelope::getGain() noexcept {
        using namespace std::chrono;
        if (m_state == State::None) {
            return 1.0f;
        }

        auto now = steady_clock::now();
        auto elapsed = duration_cast<milliseconds>(now - m_startTime);
        if (elapsed >= m_duration) {
            // Fade complete
            if (m_state == State::FadeIn) {
                m_state = State::None;
                return 1.0f;
            } else {
                m_state = State::None;
                return 0.0f;
            }
        }

        float frac = static_cast<float>(elapsed.count()) / static_cast<float>(m_duration.count());
        if (m_state == State::FadeIn) {
            return frac * frac * frac;
        } else {
            float inv = 1.0f - frac;
            return inv * inv * inv;
        }
    }

} // namespace musac
