#include "jni_io_stream.h"
#include <algorithm>
#include <cstring>

namespace musac_jni {

ByteArrayStream::ByteArrayStream(const std::vector<musac::uint8_t>& bytes)
    : data(bytes), position(0), is_open_flag(true) {
}

ByteArrayStream::ByteArrayStream(std::vector<musac::uint8_t>&& bytes)
    : data(std::move(bytes)), position(0), is_open_flag(true) {
}

musac::size ByteArrayStream::read(void* ptr, musac::size size_bytes) {
    if (!is_open_flag || position >= data.size()) {
        return 0;
    }
    
    musac::size available = data.size() - position;
    musac::size to_read = std::min(size_bytes, available);
    
    std::memcpy(ptr, data.data() + position, to_read);
    position += to_read;
    
    return to_read;
}

musac::size ByteArrayStream::write(const void* ptr, musac::size size_bytes) {
    // For now, we don't support writing (decoder input is read-only)
    return 0;
}

musac::int64_t ByteArrayStream::seek(musac::int64_t offset, musac::seek_origin whence) {
    if (!is_open_flag) {
        return -1;
    }
    
    musac::int64_t new_pos;
    
    switch (whence) {
        case musac::seek_origin::set:
            new_pos = offset;
            break;
        case musac::seek_origin::cur:
            new_pos = static_cast<musac::int64_t>(position) + offset;
            break;
        case musac::seek_origin::end:
            new_pos = static_cast<musac::int64_t>(data.size()) + offset;
            break;
        default:
            return -1;
    }
    
    if (new_pos < 0 || new_pos > static_cast<musac::int64_t>(data.size())) {
        return -1;
    }
    
    position = static_cast<musac::size>(new_pos);
    return new_pos;
}

musac::int64_t ByteArrayStream::tell() {
    if (!is_open_flag) {
        return -1;
    }
    return static_cast<musac::int64_t>(position);
}

musac::int64_t ByteArrayStream::get_size() {
    if (!is_open_flag) {
        return -1;
    }
    return static_cast<musac::int64_t>(data.size());
}

void ByteArrayStream::close() {
    is_open_flag = false;
}

bool ByteArrayStream::is_open() const {
    return is_open_flag;
}

void ByteArrayStream::append_data(const std::vector<uint8_t>& more_data) {
    data.insert(data.end(), more_data.begin(), more_data.end());
}

void ByteArrayStream::reset() {
    position = 0;
}

std::unique_ptr<musac::io_stream> create_stream_from_bytes(JNIEnv* env, jbyteArray data) {
    auto bytes = jbytearray_to_vector(env, data);
    std::vector<musac::uint8_t> musac_bytes(bytes.begin(), bytes.end());
    return std::make_unique<ByteArrayStream>(std::move(musac_bytes));
}

} // namespace musac_jni