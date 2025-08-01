#ifndef MUSAC_SDK_MEMORY_H
#define MUSAC_SDK_MEMORY_H

#include <musac/sdk/types.h>
#include <cstring>
#include <algorithm>

namespace musac {

// Memory operations using standard C++ functions
inline void* memcpy(void* dst, const void* src, size n) {
    return std::memcpy(dst, src, n);
}

inline void* memset(void* s, int c, size n) {
    return std::memset(s, c, n);
}

inline void* memmove(void* dst, const void* src, size n) {
    return std::memmove(dst, src, n);
}

inline int memcmp(const void* s1, const void* s2, size n) {
    return std::memcmp(s1, s2, n);
}

// Zero memory template function
template<typename T>
inline void zero(T& obj) {
    std::memset(&obj, 0, sizeof(T));
}

template<typename T>
inline void zero(T* ptr, size count) {
    std::memset(ptr, 0, sizeof(T) * count);
}

} // namespace musac

#endif // MUSAC_SDK_MEMORY_H