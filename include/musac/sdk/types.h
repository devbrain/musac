#ifndef MUSAC_SDK_TYPES_H
#define MUSAC_SDK_TYPES_H

#ifdef __cplusplus
#include <cstdint>
#include <cstddef>
#else
#include <stdint.h>
#include <stddef.h>
#endif

#ifdef __cplusplus
namespace musac {

// Basic integer types
using int8 = int8_t;
using int16 = int16_t;
using int32 = int32_t;
using int64 = int64_t;
using uint8 = uint8_t;
using uint16 = uint16_t;
using uint32 = uint32_t;
using uint64 = uint64_t;

// Size type
using size = std::size_t;

// Pointer difference type
using ptrdiff = std::ptrdiff_t;

} // namespace musac
#endif

#endif // MUSAC_SDK_TYPES_H