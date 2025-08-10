#ifndef MUSAC_PC_SPEAKER_DECODER_HH
#define MUSAC_PC_SPEAKER_DECODER_HH

#include <musac/sdk/decoder.hh>
#include <musac/pc_speaker_stream.hh>
#include <mutex>
#include <deque>

namespace musac {
    
    class pc_speaker_decoder : public decoder {
    public:
        pc_speaker_decoder(pc_speaker_stream* parent);
        ~pc_speaker_decoder() override;
        
        // decoder interface
        void open(io_stream* stream) override;
        [[nodiscard]] channels_t get_channels() const override;
        [[nodiscard]] sample_rate_t get_rate() const override;
        [[nodiscard]] const char* get_name() const override;
        bool rewind() override;
        [[nodiscard]] std::chrono::microseconds duration() const override;
        bool seek_to_time(std::chrono::microseconds pos) override;
        
    protected:
        size_t do_decode(float* buf, size_t len, bool& call_again) override;
        
    private:
        pc_speaker_stream* m_parent;
        sample_rate_t m_sample_rate;
        
        // Square wave generator state
        float m_phase;
        float m_phase_increment;
        float m_current_frequency;
        
        // Current tone being played
        struct {
            float frequency_hz;
            size_t samples_remaining;
            bool active;
        } m_current_tone;
        
        // Helper methods
        void set_frequency(float hz);
        float generate_sample();
        bool dequeue_next_tone();
    };
    
} // namespace musac

#endif // MUSAC_PC_SPEAKER_DECODER_HH