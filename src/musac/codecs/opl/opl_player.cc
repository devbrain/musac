//
// Created by igor on 3/20/25.
//

#include <SDL3/SDL.h>
#include <cstring>

#include "opl_player.hh"
#include <musac/sdk/samples_converter.hh>
#include <musac/sdk/from_float_converter.hh>

namespace musac {
    opl_player::opl_player(unsigned int rate)
        : m_rate(rate),
          m_state(INITIAL_STATE),
          m_time(0),
          m_sample_remains(0) {
    }

    opl_player::~opl_player() = default;

    void opl_player::copy(const opl_command* commands, std::size_t size) {
        m_queue.copy(commands, size);
    }

    void opl_player::copy(std::vector<opl_command>&& commands) {
        m_queue.copy(std::move(commands));
    }

    unsigned int opl_player::render(float* buffer, unsigned int len) {
        if (len <= 0) {
            return len;
        }
        auto* out = SDL_stack_alloc(int16_t, len);
        const std::size_t num_pairs = len / 2;
        const auto rc = do_render(out, num_pairs);
        auto out_len = 2 * rc;
        static auto cvt = get_to_float_conveter(SDL_AUDIO_S16);
        cvt(buffer, (uint8_t*)out, (unsigned int)out_len);
        SDL_stack_free(out);
        return (unsigned int)out_len;
    }

    void opl_player::rewind() {
        m_queue.rewind();
    }

    std::size_t opl_player::do_render(int16_t* buffer, std::size_t sample_pairs) {
        static constexpr float VOLUME = 1.0;
        if (m_state == INITIAL_STATE) {
            while (!m_queue.empty()) {
                if (auto cmd = m_queue.top(); cmd.Time <= m_time) {
                    m_proc.write(cmd);
                    m_queue.pop();
                } else {
                    const double elapsed = cmd.Time - m_time;
                    m_time = cmd.Time;
                    if (auto samples_to_generate = static_cast <std::size_t>(elapsed * m_rate); samples_to_generate <= sample_pairs) {
                        m_proc.render(buffer, samples_to_generate, VOLUME);
                        m_proc.write(cmd);
                        m_queue.pop();
                        return samples_to_generate;
                    } else {
                        m_proc.render(buffer, sample_pairs, VOLUME);
                        m_sample_remains = samples_to_generate - sample_pairs;
                        m_state = REMAINS_STATE;
                        return  sample_pairs;
                    }
                }
            }
        } else if (m_state == REMAINS_STATE) {
            const auto cmd = m_queue.top();
            if (m_sample_remains <= sample_pairs) {
                m_proc.render(buffer, m_sample_remains, VOLUME);
                m_proc.write(cmd);
                m_queue.pop();
                m_state = INITIAL_STATE;
                return m_sample_remains;
            } else {
                m_proc.render(buffer, sample_pairs, VOLUME);
                m_sample_remains -= sample_pairs;
                m_state = REMAINS_STATE;
                return  sample_pairs;
            }
        }

        return 0;
    }

    opl_player::commands_queue::commands_queue()
        : m_top(0) {
    }

    void opl_player::commands_queue::copy(const opl_command* commands, std::size_t size) {
        auto old_size = m_commands.size();
        m_commands.resize(old_size + size);
        std::memcpy(m_commands.data() + old_size, commands, sizeof(opl_command) * size);
    }

    void opl_player::commands_queue::copy(std::vector<opl_command>&& commands) {
        m_commands = std::move(commands);
    }

    bool opl_player::commands_queue::empty() const {
        return m_top == m_commands.size();
    }

    const opl_command& opl_player::commands_queue::top() const {
        return m_commands[m_top];
    }

    void opl_player::commands_queue::pop() {
        m_top++;
    }

    void opl_player::commands_queue::rewind() {
        m_top = 0;
    }
}
