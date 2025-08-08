#include <musac/sdk/audio_format.hh>
#include <ostream>

namespace musac {

std::ostream& operator<<(std::ostream& os, audio_format fmt) {
    switch (fmt) {
        case audio_format::unknown:
            os << "unknown";
            break;
        case audio_format::u8:
            os << "u8";
            break;
        case audio_format::s8:
            os << "s8";
            break;
        case audio_format::s16le:
            os << "s16le";
            break;
        case audio_format::s16be:
            os << "s16be";
            break;
        case audio_format::s32le:
            os << "s32le";
            break;
        case audio_format::s32be:
            os << "s32be";
            break;
        case audio_format::f32le:
            os << "f32le";
            break;
        case audio_format::f32be:
            os << "f32be";
            break;
        default:
            os << "audio_format(" << static_cast<int>(fmt) << ")";
            break;
    }
    return os;
}

} // namespace musac