#ifndef MUSAC_MIDI_SEQUENCE_HH
#define MUSAC_MIDI_SEQUENCE_HH

#include <musac/sdk/export_musac_sdk.h>
#include <musac/sdk/types.hh>
#include <cstddef>
#include <memory>

namespace musac {

class io_stream;
class opl_midi_synth;

class MUSAC_SDK_EXPORT midi_sequence {
public:
    midi_sequence();
    virtual ~midi_sequence();

    static std::unique_ptr<midi_sequence> load(const char* path);
    static std::unique_ptr<midi_sequence> load(FILE* file, int offset = 0, size_t size = 0);
    static std::unique_ptr<midi_sequence> load(io_stream* stream, int offset = 0, size_t size = 0);
    static std::unique_ptr<midi_sequence> load(const uint8_t* data, size_t size);

    virtual void reset() { m_at_end = false; }
    virtual uint32_t update(opl_midi_synth& synth) = 0;

    virtual void set_song_num(unsigned num) {
        if (num < num_songs())
            m_song_num = num;
        reset();
    }
    
    virtual unsigned num_songs() const { return 1; }
    unsigned song_num() const { return m_song_num; }
    bool at_end() const { return m_at_end; }

protected:
    bool m_at_end;
    unsigned m_song_num;

    virtual void read(const uint8_t* data, size_t size) = 0;
};

} // namespace musac

#endif // MUSAC_MIDI_SEQUENCE_HH