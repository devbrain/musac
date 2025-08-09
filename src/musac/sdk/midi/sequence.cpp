
#include "sequence.h"
#include "sequence_hmi.h"
#include "sequence_hmp.h"
#include "sequence_mid.h"
#include "sequence_mus.h"
#include "sequence_xmi.h"
#include <cstdio>
#include <vector>

namespace musac {

// ----------------------------------------------------------------------------
midi_sequence_impl::~midi_sequence_impl() {}

// ----------------------------------------------------------------------------
std::unique_ptr<midi_sequence_impl> midi_sequence_impl::load(const char* path)
{
    FILE* file = fopen(path, "rb");
    if (!file) return nullptr;

    auto seq = load(file);
    fclose(file);
    return seq;
}

// ----------------------------------------------------------------------------
std::unique_ptr<midi_sequence_impl> midi_sequence_impl::load(FILE* file, int offset, size_t size)
{
    if (!size)
    {
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

std::unique_ptr<midi_sequence_impl> midi_sequence_impl::load(musac::io_stream* stream, int offset, size_t size) {
    if (!size)
    {
        stream->seek(offset, musac::seek_origin::end);
        auto p = stream->tell();
        if (p < 0)
            return nullptr;
        size = p - offset;
    }

    stream->seek(offset, musac::seek_origin::set);
    std::vector<uint8_t> data(size);

    if (stream->read(data.data(), size) != size)
        return nullptr;

    return load(data.data(), size);
}

// ----------------------------------------------------------------------------
std::unique_ptr<midi_sequence_impl> midi_sequence_impl::load(const uint8_t* data, size_t size)
{
    std::unique_ptr<midi_sequence_impl> seq = nullptr;

    if (midi_sequence_mus::is_valid(data, size))
        seq = std::make_unique<midi_sequence_mus>();
    else if (midi_sequence_mid::is_valid(data, size))
        seq = std::make_unique<midi_sequence_mid>();
    else if (midi_sequence_xmi::is_valid(data, size))
        seq = std::make_unique<midi_sequence_xmi>();
    else if (midi_sequence_hmi::is_valid(data, size))
        seq = std::make_unique<midi_sequence_hmi>();
    else if (midi_sequence_hmp::is_valid(data, size))
        seq = std::make_unique<midi_sequence_hmp>();

    if (seq)
    {
        seq->read(data, size);
        seq->reset();
    }

    return seq;
}

} // namespace musac