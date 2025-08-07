#ifndef MUSAC_BACKENDS_NULL_BACKEND_HH
#define MUSAC_BACKENDS_NULL_BACKEND_HH

#include <memory>
#include <musac/export_musac.h>

// Public factory header for Null backend
// This header can be included by applications that want to use the Null backend for testing

namespace musac {

// Forward declaration
class audio_backend_v2;

/**
 * Create a Null audio backend instance.
 * 
 * The Null backend provides:
 * - No actual audio output (silent operation)
 * - Useful for testing without audio hardware
 * - Headless environment support
 * - Mock device enumeration for testing
 * - All operations succeed but do nothing
 * 
 * @return New Null backend instance
 * 
 * @note The backend must be initialized by calling init() before use
 * 
 * Example usage:
 * @code
 * auto backend = musac::create_null_backend_v2();
 * backend->init();
 * // Use for testing without actual audio
 * @endcode
 */
MUSAC_EXPORT std::unique_ptr<audio_backend_v2> create_null_backend_v2();

} // namespace musac

#endif // MUSAC_BACKENDS_NULL_BACKEND_HH