#ifndef MUSAC_MIDI_SEQUENCE_IMPL_H
#define MUSAC_MIDI_SEQUENCE_IMPL_H

#include <musac/sdk/types.hh>
#include <musac/sdk/io_stream.hh>
#include <memory>

namespace musac { 

// Forward declarations - we need to access the impl class
class opl_midi_synth;

/**
 * Internal base class for MIDI sequence format implementations.
 * This is used internally by the MIDI sequence system to handle
 * different MIDI file formats (MID, MUS, XMI, HMI, HMP).
 */
class midi_sequence_impl
{
public:
    midi_sequence_impl()
    {
        m_at_end = false;
        m_song_num = 0;
    }
    virtual ~midi_sequence_impl();

    // Load a sequence from the given path/file
    static std::unique_ptr<midi_sequence_impl> load(const char* path);
    static std::unique_ptr<midi_sequence_impl> load(FILE* file, int offset = 0, size_t size = 0);
    static std::unique_ptr<midi_sequence_impl> load(musac::io_stream* stream, int offset = 0, size_t size = 0);
    static std::unique_ptr<midi_sequence_impl> load(const uint8_t* data, size_t size);

    // Reset track to beginning
    virtual void reset() { m_at_end = false; }

    // Process and play any pending MIDI events
    // Returns the number of output audio samples until the next event(s)  
    virtual uint32_t update(opl_midi_synth& player) = 0;

    virtual void set_song_num(unsigned num)
    {
        if (num < num_songs())
            m_song_num = num;
        reset();
    }
    virtual unsigned num_songs() const { return 1; }
    unsigned song_num() const { return m_song_num; }

    // Has this track reached the end?
    // (this is true immediately after ending/looping, then becomes false after updating again)
    bool at_end() const { return m_at_end; }

protected:
    bool m_at_end;
    unsigned m_song_num;

private:
    virtual void read(const uint8_t* data, size_t size) = 0;
};

} // namespace musac

#endif // MUSAC_MIDI_SEQUENCE_IMPL_H

