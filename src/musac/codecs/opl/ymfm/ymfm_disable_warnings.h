//
// Created by igor on 4/10/25.
//

#ifndef YMFM_DISABLE_WARNINGS_H
#define YMFM_DISABLE_WARNINGS_H

#include <common/warn/compiler.hh>
#include <common/warn/int-float-conversion.hh>
#include <common/warn/sign-compare.hh>
#include <common/warn/sign-conversion.hh>

#if defined(BRIX_COMPILER_CLANG) || defined(BRIX_COMPILER_WASM)
# pragma clang diagnostic ignored "-Wunused-parameter"
# pragma clang diagnostic ignored "-Wimplicit-int-conversion"
#endif

#if defined(BRIX_COMPILER_GCC)
# pragma GCC diagnostic ignored "-Wunused-parameter"
# pragma GCC diagnostic ignored "-Wconversion"
# pragma GCC diagnostic ignored "-Wfloat-conversion"
#endif

#if defined(BRIX_COMPILER_MSVC)
# pragma warning(disable : 4100 4242 4265 4127)
#endif

#endif
