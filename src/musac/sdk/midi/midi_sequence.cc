#include <musac/sdk/midi/midi_sequence.hh>
#include <musac/sdk/midi/opl_midi_synth.hh>
#include <musac/sdk/io_stream.hh>
#include "sequence.h"
#include <cstdio>
#include <cstring>
#include <vector>

namespace musac {

// Wrapper class to bridge public API and internal implementation
class midi_sequence_wrapper : public midi_sequence {
private:
    std::unique_ptr<midi_sequence_impl> m_impl;

public:
    midi_sequence_wrapper(std::unique_ptr<midi_sequence_impl> impl) 
        : m_impl(std::move(impl)) {}

    void reset() override { 
        if (m_impl) m_impl->reset();
    }

    uint32_t update(opl_midi_synth& synth) override {
        if (!m_impl) return 0;
        // This will need to be implemented to bridge the synth interface
        // For now, return 0 to indicate end
        return 0;
    }

    void set_song_num(unsigned num) override {
        if (m_impl) m_impl->set_song_num(num);
    }

    unsigned num_songs() const override {
        return m_impl ? m_impl->num_songs() : 0;
    }

protected:
    void read(const uint8_t* data, size_t size) override {
        // This is handled by the impl during construction
    }
};

midi_sequence::midi_sequence()
    : m_at_end(false), m_song_num(0) {
}

midi_sequence::~midi_sequence() = default;

std::unique_ptr<midi_sequence> midi_sequence::load(const char* path) {
    FILE* file = fopen(path, "rb");
    if (!file) return nullptr;
    
    auto seq = load(file);
    fclose(file);
    
    return seq;
}

std::unique_ptr<midi_sequence> midi_sequence::load(FILE* file, int offset, size_t size) {
    if (!size) {
        fseek(file, 0, SEEK_END);
        if (ftell(file) < 0)
            return nullptr;
        size = ftell(file) - offset;
    }
    
    fseek(file, offset, SEEK_SET);
    std::vector<uint8_t> data(size);
    if (fread(data.data(), 1, size, file) != size)
        return nullptr;
    
    return load(data.data(), size);
}

std::unique_ptr<midi_sequence> midi_sequence::load(io_stream* stream, int offset, size_t size) {
    if (!size) {
        stream->seek(0, seek_origin::end);
        auto pos = stream->tell();
        if (pos < 0)
            return nullptr;
        size = pos - offset;
    }
    
    stream->seek(offset, seek_origin::set);
    std::vector<uint8_t> data(size);
    if (stream->read(data.data(), size) != size)
        return nullptr;
    
    return load(data.data(), size);
}

// This will need to be implemented to detect format and create appropriate subclass
std::unique_ptr<midi_sequence> midi_sequence::load(const uint8_t* data, size_t size) {
    // Use the internal implementation loader to detect and load the format
    auto impl = midi_sequence_impl::load(data, size);
    if (!impl) {
        return nullptr;
    }
    
    // Create a wrapper that uses the implementation
    auto sequence = std::make_unique<midi_sequence_wrapper>(std::move(impl));
    return sequence;
}

} // namespace musac