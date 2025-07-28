#pragma once

#include <chrono>

namespace musac {

    /**
     * FadeEnvelope manages a cubic fade-in and fade-out envelope over a specified duration.
     * Use startFadeIn or startFadeOut to initiate fading, then call getGain() per block
     * to retrieve the current gain [0..1].
     */
    class FadeEnvelope {
        public:
            enum class State { None, FadeIn, FadeOut };

            FadeEnvelope() = default;

            /**
             * Start a fade-in over the given duration in milliseconds.
             */
            void startFadeIn(std::chrono::milliseconds duration) noexcept;

            /**
             * Start a fade-out over the given duration in milliseconds.
             */
            void startFadeOut(std::chrono::milliseconds duration) noexcept;

            /**
             * Compute the current gain based on elapsed time since fade start.
             * Returns a value in [0,1]. If state is None, returns 1.
             * Call this once per audio block.
             */
            float getGain() noexcept;

            /**
             * Query the current fade state.
             */
            [[nodiscard]] State getState() const noexcept { return m_state; }

            void reset() {
                m_state = State::None;
            }

        private:
            State                       m_state      = State::None;
            std::chrono::milliseconds   m_duration    {0};
            std::chrono::steady_clock::time_point m_startTime;
    };

} // namespace musac
