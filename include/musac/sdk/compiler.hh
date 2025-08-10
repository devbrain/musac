//
// Created by igor on 7/15/25.
//

/**
 * @file compiler.hh
 * @brief Compiler detection and feature macros
 */
#pragma once

#if !defined(__EMSCRIPTEN__)
#if defined(__clang__)
#define MUSAC_COMPILER_CLANG
#elif defined(__GNUC__) || defined(__GNUG__)
#define MUSAC_COMPILER_GCC
#elif defined(_MSC_VER)
#define MUSAC_COMPILER_MSVC
#endif
#else
#define MUSAC_COMPILER_WASM
#endif
