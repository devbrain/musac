//
// Created by igor on 3/20/25.
//

#include <musac/sdk/audio_format.hh>
#include <cstring>
#include <alloca.h>

#include <musac/sdk/opl/opl_player.hh>
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
        auto* out = (int16_t*)alloca(len * sizeof(int16_t));
        const std::size_t num_pairs = len / 2;
        const auto rc = do_render(out, num_pairs);
        auto out_len = 2 * rc;
        static auto cvt = get_to_float_conveter(audio_s16sys);
        cvt(buffer, (uint8_t*)out, (unsigned int)out_len);
        return (unsigned int)out_len;
    }

    void opl_player::rewind() {
        m_queue.rewind();
        m_state = INITIAL_STATE;
        m_time = 0;
        m_sample_remains = 0;
        m_proc.midi_notes_clear();  // Clear all notes
        
        // Reinitialize OPL registers by replaying initial commands
        // This ensures a clean state
    }
    
    double opl_player::get_duration() const {
        return m_queue.get_duration();
    }
    
    bool opl_player::seek(double time_seconds) {
        // For seeking, we need to rebuild OPL state by replaying commands
        // up to the target time point
        
        // Clear current state
        m_proc.midi_notes_clear();
        
        // Reset queue to beginning
        m_queue.rewind();
        
        // Replay all commands up to the target time instantly
        while (!m_queue.empty()) {
            const auto& cmd = m_queue.top();
            if (cmd.Time >= time_seconds) {
                break;  // We've reached or passed the target time
            }
            m_proc.write(cmd);
            m_queue.pop();
        }
        
        // Set the time position
        m_time = time_seconds;
        m_state = INITIAL_STATE;
        m_sample_remains = 0;
        
        return true;
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
    
    double opl_player::commands_queue::get_duration() const {
        if (m_commands.empty()) {
            return 0.0;
        }
        // The duration is the time of the last command
        return m_commands.back().Time;
    }
    
    bool opl_player::commands_queue::seek(double time_seconds, std::size_t& index) {
        // Binary search to find the command index at or just before the target time
        std::size_t left = 0;
        std::size_t right = m_commands.size();
        
        while (left < right) {
            std::size_t mid = left + (right - left) / 2;
            if (m_commands[mid].Time <= time_seconds) {
                left = mid + 1;
            } else {
                right = mid;
            }
        }
        
        // Set the index to the position just before or at the target time
        if (left > 0) {
            index = left;
            m_top = left;
            return true;
        }
        
        // If seeking to the beginning
        index = 0;
        m_top = 0;
        return true;
    }
}
