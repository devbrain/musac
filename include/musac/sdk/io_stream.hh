/**
 * @file io_stream.hh
 * @brief Binary I/O stream abstraction
 * @ingroup sdk_io
 */

#ifndef MUSAC_SDK_IO_STREAM_H
#define MUSAC_SDK_IO_STREAM_H

#include <musac/sdk/types.hh>
#include <musac/sdk/export_musac_sdk.h>
#include <memory>

namespace musac {

/**
 * @enum seek_origin
 * @brief Seek origin for stream positioning
 * @ingroup sdk_io
 */
enum class seek_origin : int {
    set = 0,  ///< Seek from beginning of stream (SEEK_SET)
    cur = 1,  ///< Seek from current position (SEEK_CUR)
    end = 2   ///< Seek from end of stream (SEEK_END)
};

/**
 * @class io_stream
 * @brief Abstract interface for binary I/O operations
 * @ingroup sdk_io
 * 
 * The io_stream class provides a custom I/O abstraction optimized for
 * binary audio data. Unlike std::istream, it focuses on binary operations
 * with direct support for endian-aware reading.
 * 
 * ## Why Not std::istream?
 * 
 * - **Binary-focused**: No text/formatting overhead
 * - **Endian support**: Built-in helpers for LE/BE reading
 * - **Size awareness**: get_size() for buffer allocation
 * - **Simple API**: Only 8 essential methods
 * - **Zero-copy**: Memory streams without string overhead
 * - **C compatible**: Can wrap C APIs (FILE*, SDL_RWops)
 * 
 * ## Implementation Examples
 * 
 * @code
 * class file_stream : public io_stream {
 *     FILE* m_file;
 * public:
 *     size_t read(void* ptr, size_t size) override {
 *         return fread(ptr, 1, size, m_file);
 *     }
 *     // ... other methods
 * };
 * @endcode
 * 
 * ## Using I/O Streams
 * 
 * @code
 * // Open file stream
 * auto stream = io_from_file("audio.wav", "rb");
 * 
 * // Read WAV header
 * uint32_t chunk_id;
 * if (!read_u32be(stream.get(), &chunk_id)) {
 *     // Handle error
 * }
 * 
 * // Create memory stream
 * std::vector<uint8_t> data = load_data();
 * auto mem_stream = io_from_memory(data.data(), data.size());
 * @endcode
 * 
 * @see io_from_file(), io_from_memory(), decoder
 */
class io_stream {
public:
    /**
     * @brief Virtual destructor
     */
    virtual ~io_stream() = default;
    
    /**
     * @brief Read binary data from stream
     * 
     * @param ptr Buffer to read into
     * @param size_bytes Number of bytes to read
     * @return Actual number of bytes read (may be less than requested)
     * 
     * @note Returns 0 on EOF or error
     */
    virtual size_t read(void* ptr, size_t size_bytes) = 0;
    
    /**
     * @brief Write binary data to stream
     * 
     * @param ptr Data to write
     * @param size_bytes Number of bytes to write
     * @return Actual number of bytes written
     * 
     * @note Not all streams support writing
     */
    virtual size_t write(const void* ptr, size_t size_bytes) = 0;
    
    /**
     * @brief Seek to a position in the stream
     * 
     * @param offset Byte offset from origin
     * @param whence Origin for seek operation
     * @return New position from start, or -1 on error
     * 
     * @code
     * // Seek to beginning
     * stream->seek(0, seek_origin::set);
     * 
     * // Skip 100 bytes forward
     * stream->seek(100, seek_origin::cur);
     * 
     * // Seek to last 4 bytes
     * stream->seek(-4, seek_origin::end);
     * @endcode
     */
    virtual int64_t seek(int64_t offset, seek_origin whence) = 0;
    
    /**
     * @brief Get current position in stream
     * @return Current byte position from start, or -1 on error
     */
    virtual int64_t tell() = 0;
    
    /**
     * @brief Get total size of stream
     * @return Total size in bytes, or -1 if unknown/unlimited
     * @note Useful for progress reporting and buffer allocation
     */
    virtual int64_t get_size() = 0;
    
    /**
     * @brief Close the stream
     * @note After closing, all operations except is_open() are undefined
     */
    virtual void close() = 0;
    
    /**
     * @brief Check if stream is open and usable
     * @return true if stream is open
     */
    [[nodiscard]] virtual bool is_open() const = 0;
};

/**
 * @defgroup io_endian Endian-aware I/O helpers
 * @ingroup sdk_io
 * @brief Convenience functions for reading binary data with endian conversion
 * 
 * These functions read integers from streams and handle endian conversion
 * automatically. Essential for parsing audio file formats which may use
 * different byte orders.
 * 
 * @{
 */

/**
 * @brief Read unsigned 8-bit value
 * @param stream Stream to read from
 * @param[out] value Value read
 * @return true on success, false on error
 */
MUSAC_SDK_EXPORT bool read_u8(io_stream* stream, uint8_t* value);

/**
 * @brief Read unsigned 16-bit little-endian value
 * @param stream Stream to read from
 * @param[out] value Value read (converted to native endian)
 * @return true on success, false on error
 */
MUSAC_SDK_EXPORT bool read_u16le(io_stream* stream, uint16_t* value);

/**
 * @brief Read unsigned 16-bit big-endian value
 * @param stream Stream to read from
 * @param[out] value Value read (converted to native endian)
 * @return true on success, false on error
 */
MUSAC_SDK_EXPORT bool read_u16be(io_stream* stream, uint16_t* value);

/**
 * @brief Read unsigned 32-bit little-endian value
 * @param stream Stream to read from
 * @param[out] value Value read (converted to native endian)
 * @return true on success, false on error
 * 
 * @code
 * // Read WAV chunk size (little-endian)
 * uint32_t chunk_size;
 * if (!read_u32le(stream.get(), &chunk_size)) {
 *     // Handle error
 * }
 * @endcode
 */
MUSAC_SDK_EXPORT bool read_u32le(io_stream* stream, uint32_t* value);

/**
 * @brief Read unsigned 32-bit big-endian value
 * @param stream Stream to read from
 * @param[out] value Value read (converted to native endian)
 * @return true on success, false on error
 */
MUSAC_SDK_EXPORT bool read_u32be(io_stream* stream, uint32_t* value);

/**
 * @brief Read signed 16-bit little-endian value
 * @param stream Stream to read from
 * @param[out] value Value read (converted to native endian)
 * @return true on success, false on error
 */
MUSAC_SDK_EXPORT bool read_s16le(io_stream* stream, int16_t* value);

/**
 * @brief Read signed 16-bit big-endian value
 * @param stream Stream to read from
 * @param[out] value Value read (converted to native endian)
 * @return true on success, false on error
 */
MUSAC_SDK_EXPORT bool read_s16be(io_stream* stream, int16_t* value);

/**
 * @brief Read signed 32-bit little-endian value
 * @param stream Stream to read from
 * @param[out] value Value read (converted to native endian)
 * @return true on success, false on error
 */
MUSAC_SDK_EXPORT bool read_s32le(io_stream* stream, int32_t* value);

/**
 * @brief Read signed 32-bit big-endian value
 * @param stream Stream to read from
 * @param[out] value Value read (converted to native endian)
 * @return true on success, false on error
 */
MUSAC_SDK_EXPORT bool read_s32be(io_stream* stream, int32_t* value);

/** @} */ // end of io_endian group

/**
 * @defgroup io_factory I/O Stream Factory Functions
 * @ingroup sdk_io
 * @brief Factory functions for creating I/O streams
 * @{
 */

/**
 * @brief Create a file-based I/O stream
 * 
 * @param filename Path to file
 * @param mode File mode ("rb" for read binary, "wb" for write binary)
 * @return New io_stream, or nullptr on error
 * 
 * @code
 * auto stream = io_from_file("audio.wav", "rb");
 * if (!stream) {
 *     // Handle error
 * }
 * @endcode
 */
MUSAC_SDK_EXPORT std::unique_ptr<io_stream> io_from_file(const char* filename, const char* mode);

/**
 * @brief Create a read-only memory-based I/O stream
 * 
 * @param mem Pointer to memory buffer
 * @param size_bytes Size of buffer in bytes
 * @return New io_stream
 * 
 * @note The memory must remain valid for the lifetime of the stream
 * 
 * @code
 * std::vector<uint8_t> data = load_audio_data();
 * auto stream = io_from_memory(data.data(), data.size());
 * @endcode
 */
MUSAC_SDK_EXPORT std::unique_ptr<io_stream> io_from_memory(const void* mem, size_t size_bytes);

/**
 * @brief Create a read-write memory-based I/O stream
 * 
 * @param mem Pointer to mutable memory buffer
 * @param size_bytes Size of buffer in bytes
 * @return New io_stream
 * 
 * @note The memory must remain valid for the lifetime of the stream
 */
MUSAC_SDK_EXPORT std::unique_ptr<io_stream> io_from_memory_rw(void* mem, size_t size_bytes);

/** @} */ // end of io_factory group

} // namespace musac

#endif // MUSAC_SDK_IO_STREAM_H