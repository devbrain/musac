//
// Legacy compatibility header - use chip_emulator.hh instead
//

#ifndef YMFM_CHIP_HH
#define YMFM_CHIP_HH

#include <musac/sdk/opl/chip_emulator.hh>
#include <memory>
#include <vector>
#include <utility>
#include <stdexcept>

namespace musac {
    // Legacy type alias for compatibility
    using emulated_time = int64_t;
    
    // Map old chip_type enum to new chip_type_id
    enum chip_type {
        CHIP_YM2149  = static_cast<int>(chip_type_id::YM2149),
        CHIP_YM2151  = static_cast<int>(chip_type_id::YM2151),
        CHIP_YM2203  = static_cast<int>(chip_type_id::YM2203),
        CHIP_YM2413  = static_cast<int>(chip_type_id::YM2413),
        CHIP_YM2608  = static_cast<int>(chip_type_id::YM2608),
        CHIP_YM2610  = static_cast<int>(chip_type_id::YM2610),
        CHIP_YM2612  = static_cast<int>(chip_type_id::YM2612),
        CHIP_YM3526  = static_cast<int>(chip_type_id::YM3526),
        CHIP_Y8950   = static_cast<int>(chip_type_id::Y8950),
        CHIP_YM3812  = static_cast<int>(chip_type_id::YM3812),
        CHIP_YMF262  = static_cast<int>(chip_type_id::YMF262),
        CHIP_YMF278B = static_cast<int>(chip_type_id::YMF278B),
        CHIP_TYPES   = static_cast<int>(chip_type_id::CHIP_TYPES_COUNT)
    };
    
    // Legacy base class for compatibility
    // New code should use chip_emulator directly
    class ymfm_chip_base {
    public:
        ymfm_chip_base(uint32_t clock, chip_type type, const char* name)
            : m_clock(clock)
            , m_type(type)
            , m_name(name ? name : "")
            , m_emulator(create_chip_emulator(static_cast<chip_type_id>(type), clock))
            , m_pos(0)
            , m_queue()
            , m_output{0} {
            if (!m_emulator) {
                // Failed to create emulator
                throw std::runtime_error("Failed to create chip emulator");
            }
            // Reset the chip to ensure it's in a known state
            m_emulator->reset();
            
            // Calculate the step size for timing
            uint32_t sample_rate = m_emulator->sample_rate();
            if (sample_rate > 0) {
                m_step = 0x100000000ull / sample_rate;
            } else {
                m_step = 0x100000000ull / 44100; // Default to 44.1kHz
            }
        }
        
        virtual ~ymfm_chip_base() = default;
        
        [[nodiscard]] chip_type type() const { 
            return m_type; 
        }
        
        [[nodiscard]] virtual uint32_t sample_rate() const {
            return m_emulator ? m_emulator->sample_rate() : 0;
        }
        
        virtual void reset() {
            if (m_emulator) {
                m_emulator->reset();
            }
        }
        
        virtual void write(uint32_t reg, uint8_t data) {
            // Queue the write for processing during generate
            // This ensures proper synchronization with sample generation
            m_queue.emplace_back(reg, data);
        }
        
        virtual void generate(int32_t* buffer, uint32_t count) {
            if (m_emulator) {
                m_emulator->generate(buffer, count);
            }
        }
        
        void set_silent_mode(bool enable) {
            if (m_emulator) {
                m_emulator->set_silent_mode(enable);
            }
        }
        
        bool get_silent_mode() const {
            return m_emulator ? m_emulator->get_silent_mode() : false;
        }
        
        // Legacy generate method for VGM player compatibility
        // This generates samples with proper timing to match the original implementation
        virtual void generate(emulated_time output_start, emulated_time output_step, int32_t* buffer) {
            if (m_emulator) {
                uint32_t addr1 = 0xffff, addr2 = 0xffff;
                uint8_t data1 = 0, data2 = 0;
                
                // Process one queued write if available
                if (!m_queue.empty()) {
                    auto front = m_queue.front();
                    addr1 = 0 + 2 * ((front.first >> 8) & 3);
                    data1 = front.first & 0xff;
                    addr2 = addr1 + ((m_type == CHIP_YM2149) ? 2 : 1);
                    data2 = front.second;
                    m_queue.erase(m_queue.begin());
                }
                
                // Write to the chip
                if (addr1 != 0xffff) {
                    m_emulator->write(addr1, data1);
                    m_emulator->write(addr2, data2);
                }
                
                // Generate samples until we catch up to the requested output position
                uint32_t num_channels = m_emulator->num_outputs();
                for (; m_pos <= output_start; m_pos += m_step) {
                    m_emulator->generate(m_output, num_channels);
                }
                
                // Add the final result to the buffer using pointer increment
                if (m_type == CHIP_YM2203) {
                    // YM2203 has special mixing
                    int32_t out0 = m_output[0];
                    int32_t out1 = (num_channels > 1) ? m_output[1] : 0;
                    int32_t out2 = (num_channels > 2) ? m_output[2] : 0;
                    int32_t out3 = (num_channels > 3) ? m_output[3] : 0;
                    *buffer++ += out0 + out1 + out2 + out3;
                    *buffer++ += out0 + out1 + out2 + out3;
                } else if (m_type == CHIP_YM2608 || m_type == CHIP_YM2610) {
                    // YM2608/YM2610 have special mixing
                    int32_t out0 = m_output[0];
                    int32_t out1 = (num_channels > 1) ? m_output[1] : 0;
                    int32_t out2 = (num_channels > 2) ? m_output[2] : 0;
                    *buffer++ += out0 + out2;
                    *buffer++ += out1 + out2;
                } else if (m_type == CHIP_YMF278B) {
                    // YMF278B uses channels 4 and 5 for output
                    *buffer++ += (num_channels > 4) ? m_output[4] : 0;
                    *buffer++ += (num_channels > 5) ? m_output[5] : 0;
                } else if (num_channels == 1) {
                    // Mono output - duplicate to both channels
                    *buffer++ += m_output[0];
                    *buffer++ += m_output[0];
                } else {
                    // Stereo output
                    *buffer++ += m_output[0];
                    *buffer++ += (num_channels > 1) ? m_output[1] : m_output[0];
                }
            }
        }
        
        // Stub methods for PCM support (not all chips support these)
        virtual void write_data(int access, uint32_t start, uint32_t size, const uint8_t* data) {
            // Not implemented for most chips
            (void)access; (void)start; (void)size; (void)data;
        }
        
        virtual uint8_t read_pcm() {
            // Not implemented for most chips
            return 0;
        }
        
        virtual void seek_pcm(uint32_t pos) {
            // Not implemented for most chips
            (void)pos;
        }
        
    protected:
        uint32_t m_clock;
        chip_type m_type;
        std::string m_name;
        std::unique_ptr<chip_emulator> m_emulator;
        emulated_time m_pos;  // Current position for timing
        emulated_time m_step; // Step size based on sample rate
        std::vector<std::pair<uint32_t, uint8_t>> m_queue; // Write queue
        int32_t m_output[16]; // Output buffer for chip samples
    };
    
    // Template for compatibility with existing code
    // This is a facade that delegates to chip_emulator
    template<typename ChipType>
    class ymfm_chip : public ymfm_chip_base {
    public:
        ymfm_chip(uint32_t clock, chip_type type, const char* name)
            : ymfm_chip_base(clock, type, name) {
        }
        
        // Expose the inherited generate method from base class
        using ymfm_chip_base::generate;
        
        void generate(int32_t* buffer) {
            if (m_emulator) {
                m_emulator->generate(buffer, m_emulator->num_outputs());
            }
        }
    };
}

#endif // YMFM_CHIP_HH