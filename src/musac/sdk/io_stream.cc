#include <musac/sdk/io_stream.hh>
#include <musac/sdk/endian.hh>
#include <fstream>
#include <vector>
#include <cstring>

namespace musac {

// Memory stream implementation
class memory_stream : public io_stream {
public:
    memory_stream(const void* data, size size_bytes, bool writable = false)
        : m_data(static_cast<const uint8*>(data))
        , m_writable_data(writable ? static_cast<uint8*>(const_cast<void*>(data)) : nullptr)
        , m_size(size_bytes)
        , m_position(0)
        , m_is_open(true) {}
    
    size read(void* ptr, size size_bytes) override {
        if (!m_is_open || m_position >= m_size) {
            return 0;
        }
        
        size to_read = std::min(size_bytes, m_size - m_position);
        std::memcpy(ptr, m_data + m_position, to_read);
        m_position += to_read;
        return to_read;
    }
    
    size write(const void* ptr, size size_bytes) override {
        if (!m_is_open || !m_writable_data || m_position >= m_size) {
            return 0;
        }
        
        size to_write = std::min(size_bytes, m_size - m_position);
        std::memcpy(m_writable_data + m_position, ptr, to_write);
        m_position += to_write;
        return to_write;
    }
    
    int64 seek(int64 offset, seek_origin whence) override {
        if (!m_is_open) {
            return -1;
        }
        
        int64 new_pos = 0;
        switch (whence) {
            case seek_origin::set:
                new_pos = offset;
                break;
            case seek_origin::cur:
                new_pos = static_cast<int64>(m_position) + offset;
                break;
            case seek_origin::end:
                new_pos = static_cast<int64>(m_size) + offset;
                break;
        }
        
        if (new_pos < 0 || new_pos > static_cast<int64>(m_size)) {
            return -1;
        }
        
        m_position = static_cast<size>(new_pos);
        return new_pos;
    }
    
    int64 tell() override {
        return m_is_open ? static_cast<int64>(m_position) : -1;
    }
    
    int64 get_size() override {
        return m_is_open ? static_cast<int64>(m_size) : -1;
    }
    
    void close() override {
        m_is_open = false;
    }
    
    bool is_open() const override {
        return m_is_open;
    }
    
private:
    const uint8* m_data;
    uint8* m_writable_data;
    size m_size;
    size m_position;
    bool m_is_open;
};

// File stream implementation
class file_stream : public io_stream {
public:
    file_stream(const char* filename, const char* mode) {
        std::ios::openmode open_mode = std::ios::binary;
        
        if (std::strchr(mode, 'r')) {
            open_mode |= std::ios::in;
            if (std::strchr(mode, '+')) {
                open_mode |= std::ios::out;
            }
        } else if (std::strchr(mode, 'w')) {
            open_mode |= std::ios::out | std::ios::trunc;
            if (std::strchr(mode, '+')) {
                open_mode |= std::ios::in;
            }
        } else if (std::strchr(mode, 'a')) {
            open_mode |= std::ios::out | std::ios::app;
            if (std::strchr(mode, '+')) {
                open_mode |= std::ios::in;
            }
        }
        
        m_file.open(filename, open_mode);
    }
    
    size read(void* ptr, size size_bytes) override {
        if (!is_open()) {
            return 0;
        }
        m_file.read(static_cast<char*>(ptr), size_bytes);
        return m_file.gcount();
    }
    
    size write(const void* ptr, size size_bytes) override {
        if (!is_open()) {
            return 0;
        }
        m_file.write(static_cast<const char*>(ptr), size_bytes);
        return m_file.good() ? size_bytes : 0;
    }
    
    int64 seek(int64 offset, seek_origin whence) override {
        if (!is_open()) {
            return -1;
        }
        
        std::ios::seekdir dir;
        switch (whence) {
            case seek_origin::set: dir = std::ios::beg; break;
            case seek_origin::cur: dir = std::ios::cur; break;
            case seek_origin::end: dir = std::ios::end; break;
        }
        
        m_file.seekg(offset, dir);
        m_file.seekp(offset, dir);
        return tell();
    }
    
    int64 tell() override {
        if (!is_open()) {
            return -1;
        }
        return m_file.tellg();
    }
    
    int64 get_size() override {
        if (!is_open()) {
            return -1;
        }
        auto cur_pos = m_file.tellg();
        m_file.seekg(0, std::ios::end);
        auto file_size = m_file.tellg();
        m_file.seekg(cur_pos);
        return file_size;
    }
    
    void close() override {
        m_file.close();
    }
    
    bool is_open() const override {
        return m_file.is_open();
    }
    
private:
    mutable std::fstream m_file;
};

// Convenience read functions implementation
bool read_u8(io_stream* stream, uint8* value) {
    return stream->read(value, 1) == 1;
}

bool read_u16le(io_stream* stream, uint16* value) {
    uint16 temp;
    if (stream->read(&temp, 2) != 2) return false;
    *value = swap16le(temp);
    return true;
}

bool read_u16be(io_stream* stream, uint16* value) {
    uint16 temp;
    if (stream->read(&temp, 2) != 2) return false;
    *value = swap16be(temp);
    return true;
}

bool read_u32le(io_stream* stream, uint32* value) {
    uint32 temp;
    if (stream->read(&temp, 4) != 4) return false;
    *value = swap32le(temp);
    return true;
}

bool read_u32be(io_stream* stream, uint32* value) {
    uint32 temp;
    if (stream->read(&temp, 4) != 4) return false;
    *value = swap32be(temp);
    return true;
}

bool read_s16le(io_stream* stream, int16* value) {
    return read_u16le(stream, reinterpret_cast<uint16*>(value));
}

bool read_s16be(io_stream* stream, int16* value) {
    return read_u16be(stream, reinterpret_cast<uint16*>(value));
}

bool read_s32le(io_stream* stream, int32* value) {
    return read_u32le(stream, reinterpret_cast<uint32*>(value));
}

bool read_s32be(io_stream* stream, int32* value) {
    return read_u32be(stream, reinterpret_cast<uint32*>(value));
}

// Factory functions
std::unique_ptr<io_stream> io_from_file(const char* filename, const char* mode) {
    auto stream = std::make_unique<file_stream>(filename, mode);
    if (!stream->is_open()) {
        return nullptr;
    }
    return stream;
}

std::unique_ptr<io_stream> io_from_memory(const void* mem, size size_bytes) {
    return std::make_unique<memory_stream>(mem, size_bytes, false);
}

std::unique_ptr<io_stream> io_from_memory_rw(void* mem, size size_bytes) {
    return std::make_unique<memory_stream>(mem, size_bytes, true);
}

} // namespace musac