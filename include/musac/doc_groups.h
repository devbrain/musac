/**
 * @file doc_groups.h
 * @brief Doxygen group definitions for Musac Audio Library
 * 
 * This file defines the module/group structure for the documentation.
 * It doesn't contain any actual code.
 */

#ifndef MUSAC_DOC_GROUPS_H
#define MUSAC_DOC_GROUPS_H

/**
 * @defgroup musac Musac Audio Library
 * @brief Complete audio library for games and applications
 */

/**
 * @defgroup core Core Components
 * @ingroup musac
 * @brief Essential audio system components
 * 
 * The core components provide the main functionality of the musac library:
 * - Audio device management
 * - Stream playback control  
 * - Audio source handling
 * - Audio mixing
 * - PC speaker emulation
 * 
 * @{
 */

/**
 * @defgroup devices Device Management
 * @brief Audio device enumeration and control
 * 
 * Provides functionality for:
 * - Enumerating available audio devices
 * - Opening and configuring devices
 * - Managing device lifecycle
 * - Hot-plug detection
 */

/**
 * @defgroup streams Audio Streams
 * @brief Stream playback and control
 * 
 * Audio streams handle:
 * - Playback control (play/pause/stop)
 * - Volume and stereo positioning
 * - Looping and callbacks
 * - Fade in/out effects
 * - State management
 */

/**
 * @defgroup sources Audio Sources
 * @brief Audio data sources and loading
 * 
 * Audio sources provide:
 * - File loading
 * - Memory buffers
 * - Format conversion
 * - Resampling
 */

/**
 * @defgroup mixing Audio Mixing
 * @brief Multi-stream mixing and processing
 * 
 * The mixer handles:
 * - Combining multiple streams
 * - Global volume control
 * - Thread-safe operations
 * - Real-time processing
 */

/**
 * @defgroup pc_speaker PC Speaker
 * @brief PC speaker emulation and MML support
 * 
 * PC speaker functionality includes:
 * - Tone generation
 * - MML (Music Macro Language) playback
 * - Retro game sounds
 * - Square wave synthesis
 */

/** @} */ // end of core group

/**
 * @defgroup backends Audio Backends
 * @ingroup musac
 * @brief Platform-specific audio implementations
 * 
 * Supported backends:
 * - SDL2 - Cross-platform via SDL 2.x
 * - SDL3 - Cross-platform via SDL 3.x
 * - Custom - User-provided implementations
 * 
 * @{
 */

/**
 * @defgroup backend_interface Backend Interface
 * @brief Abstract interface for audio backends
 * 
 * All backends must implement:
 * - Device enumeration
 * - Device management
 * - Stream creation
 * - Audio callbacks
 */

/**
 * @defgroup backend_sdl2 SDL2 Backend
 * @brief SDL 2.x audio backend implementation
 */

/**
 * @defgroup backend_sdl3 SDL3 Backend  
 * @brief SDL 3.x audio backend implementation
 */

/** @} */ // end of backends group

/**
 * @defgroup codecs Audio Codecs
 * @ingroup musac
 * @brief Audio format decoders
 * 
 * Supported formats:
 * - WAV - Windows Wave files
 * - MP3 - MPEG Layer 3 audio
 * - FLAC - Free Lossless Audio Codec
 * - OGG/Vorbis - Ogg Vorbis audio
 * - AIFF - Audio Interchange File Format
 * - MOD - Module tracker formats
 * - VGM - Video Game Music
 * - VOC - Creative Voice files
 * - MML - Music Macro Language
 * - And more...
 * 
 * @{
 */

/**
 * @defgroup decoder_interface Decoder Interface
 * @brief Base interface for all decoders
 * 
 * Decoder interface provides:
 * - Format detection
 * - Audio decoding
 * - Seeking support
 * - Metadata extraction
 */

/** @} */ // end of codecs group

/**
 * @defgroup sdk SDK Components
 * @ingroup musac
 * @brief Low-level SDK components
 * 
 * The SDK provides fundamental types and utilities:
 * - Type definitions
 * - I/O abstractions
 * - Buffer management
 * - Audio conversion
 * - Resampling
 * 
 * @{
 */

/**
 * @defgroup sdk_types Type Definitions
 * @brief Platform-independent type definitions
 */

/**
 * @defgroup sdk_io I/O Abstractions
 * @brief Input/output stream abstractions
 * 
 * Custom I/O system providing:
 * - Binary-focused operations
 * - Memory streams
 * - File streams
 * - Endian-aware helpers
 */

/**
 * @defgroup sdk_conversion Audio Conversion
 * @brief Format conversion and resampling
 * 
 * Conversion utilities for:
 * - Sample format conversion
 * - Sample rate conversion
 * - Channel conversion
 * - High-quality resampling
 */

/**
 * @defgroup sdk_buffers Buffer Management
 * @brief Audio buffer utilities
 */

/** @} */ // end of sdk group

/**
 * @defgroup errors Error Handling
 * @ingroup musac
 * @brief Exception hierarchy and error management
 * 
 * Error handling includes:
 * - Exception hierarchy
 * - Error codes
 * - Recovery strategies
 * - Debug helpers
 */

/**
 * @defgroup examples Examples
 * @ingroup musac
 * @brief Usage examples and tutorials
 * 
 * Example categories:
 * - Basic playback
 * - Device management
 * - Advanced features
 * - Game integration
 * - PC speaker/MML
 */

/**
 * @defgroup internal Internal Components
 * @ingroup musac
 * @brief Internal implementation details
 * 
 * @warning These are implementation details and should not be used directly.
 * 
 * Internal components include:
 * - Thread synchronization
 * - Memory management
 * - Platform-specific code
 * - Optimization utilities
 */

#endif // MUSAC_DOC_GROUPS_H