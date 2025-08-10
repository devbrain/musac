#ifndef MUSAC_JNI_IO_STREAM_H
#define MUSAC_JNI_IO_STREAM_H

#include "jni_common.h"
#include <musac/sdk/io_stream.hh>
#include <musac/sdk/types.hh>
#include <vector>

namespace musac_jni {

// io_stream implementation that wraps Java byte array
class ByteArrayStream : public musac::io_stream {
private:
    std::vector<musac::uint8_t> data;
    musac::size position;
    bool is_open_flag;
    
public:
    explicit ByteArrayStream(const std::vector<musac::uint8_t>& bytes);
    explicit ByteArrayStream(std::vector<musac::uint8_t>&& bytes);
    
    // io_stream interface
    musac::size read(void* ptr, musac::size size_bytes) override;
    musac::size write(const void* ptr, musac::size size_bytes) override;
    musac::int64_t seek(musac::int64_t offset, musac::seek_origin whence) override;
    musac::int64_t tell() override;
    musac::int64_t get_size() override;
    void close() override;
    bool is_open() const override;
    
    // Additional methods for streaming
    void append_data(const std::vector<uint8_t>& more_data);
    void reset();
};

// Create io_stream from Java byte array
std::unique_ptr<musac::io_stream> create_stream_from_bytes(JNIEnv* env, jbyteArray data);

} // namespace musac_jni

#endif // MUSAC_JNI_IO_STREAM_H