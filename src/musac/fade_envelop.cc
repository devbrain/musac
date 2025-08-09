//
// Created by igor on 4/28/25.
//

#include "musac/fade_envelop.hh"
#include <cmath>

namespace musac {

    void fade_envelope::startFadeIn(std::chrono::milliseconds duration) noexcept {
        m_duration = duration;
        m_startTime = std::chrono::steady_clock::now();
        m_state = state::fade_in;
    }

    void fade_envelope::startFadeOut(std::chrono::milliseconds duration) noexcept {
        m_duration = duration;
        m_startTime = std::chrono::steady_clock::now();
        m_state = state::fade_out;
    }

    float fade_envelope::getGain() noexcept {
        using namespace std::chrono;
        if (m_state == state::none) {
            return 1.0f;
        }

        auto now = steady_clock::now();
        auto elapsed = duration_cast<milliseconds>(now - m_startTime);
        if (elapsed >= m_duration) {
            // Fade complete
            if (m_state == state::fade_in) {
                m_state = state::none;
                return 1.0f;
            } else {
                m_state = state::none;
                return 0.0f;
            }
        }

        float frac = static_cast<float>(elapsed.count()) / static_cast<float>(m_duration.count());
        if (m_state == state::fade_in) {
            return frac * frac * frac;
        } else {
            float inv = 1.0f - frac;
            return inv * inv * inv;
        }
    }

} // namespace musac
