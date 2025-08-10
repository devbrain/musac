#pragma once

#include <musac/sdk/compiler.hh>

#if defined(MUSAC_COMPILER_MSVC)
#pragma warning( push )
#pragma warning( disable : 4820)
#elif defined(MUSAC_COMPILER_GCC)
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wold-style-cast"
# pragma GCC diagnostic ignored "-Wuseless-cast"
#elif defined(MUSAC_COMPILER_CLANG)
# pragma clang diagnostic push
#elif defined(MUSAC_COMPILER_WASM)
# pragma clang diagnostic push
#endif

#include <SDL3/SDL.h>

#if defined(MUSAC_COMPILER_MSVC)
#pragma warning( pop )
#elif defined(MUSAC_COMPILER_GCC)
# pragma GCC diagnostic pop
#elif defined(MUSAC_COMPILER_CLANG)
# pragma clang diagnostic pop
#elif defined(MUSAC_COMPILER_WASM)
# pragma clang diagnostic pop
#endif
