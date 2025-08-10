#ifndef MUSAC_SDK_IO_STREAM_H
#define MUSAC_SDK_IO_STREAM_H

#include <musac/sdk/types.hh>
#include <musac/sdk/export_musac_sdk.h>
#include <memory>

namespace musac {

// Seek origins (compatible with standard C)
enum class seek_origin : int {
    set = 0,  // SEEK_SET - Beginning of file
    cur = 1,  // SEEK_CUR - Current position
    end = 2   // SEEK_END - End of file
};

class io_stream {
public:
    virtual ~io_stream() = default;
    
    // Read up to 'size' bytes into 'ptr', returns actual bytes read
    virtual size_t read(void* ptr, size_t size_bytes) = 0;
    
    // Write up to 'size' bytes from 'ptr', returns actual bytes written
    virtual size_t write(const void* ptr, size_t size_bytes) = 0;
    
    // Seek to position, returns new position or -1 on error
    virtual int64_t seek(int64_t offset, seek_origin whence) = 0;
    
    // Get current position, returns -1 on error
    virtual int64_t tell() = 0;
    
    // Get total size, returns -1 if unknown
    virtual int64_t get_size() = 0;
    
    // Close the stream
    virtual void close() = 0;
    
    // Check if stream is open
    [[nodiscard]] virtual bool is_open() const = 0;
};

// Convenience functions for reading with endian conversion
MUSAC_SDK_EXPORT bool read_u8(io_stream* stream, uint8_t* value);
MUSAC_SDK_EXPORT bool read_u16le(io_stream* stream, uint16_t* value);
MUSAC_SDK_EXPORT bool read_u16be(io_stream* stream, uint16_t* value);
MUSAC_SDK_EXPORT bool read_u32le(io_stream* stream, uint32_t* value);
MUSAC_SDK_EXPORT bool read_u32be(io_stream* stream, uint32_t* value);
MUSAC_SDK_EXPORT bool read_s16le(io_stream* stream, int16_t* value);
MUSAC_SDK_EXPORT bool read_s16be(io_stream* stream, int16_t* value);
MUSAC_SDK_EXPORT bool read_s32le(io_stream* stream, int32_t* value);
MUSAC_SDK_EXPORT bool read_s32be(io_stream* stream, int32_t* value);

// Factory functions
MUSAC_SDK_EXPORT std::unique_ptr<io_stream> io_from_file(const char* filename, const char* mode);
MUSAC_SDK_EXPORT std::unique_ptr<io_stream> io_from_memory(const void* mem, size_t size_bytes);
MUSAC_SDK_EXPORT std::unique_ptr<io_stream> io_from_memory_rw(void* mem, size_t size_bytes);

} // namespace musac

#endif // MUSAC_SDK_IO_STREAM_H