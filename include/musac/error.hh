// This is copyrighted software. More information is at the end of this file.
#pragma once

#include <stdexcept>
#include <string>

namespace musac {

/**
 * @brief Base exception class for all musac errors
 * 
 * All musac-specific exceptions derive from this class, making it easy
 * to catch all musac errors with a single catch block.
 */
class musac_error : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

/**
 * @brief Audio device related errors
 * 
 * Thrown when device operations fail, such as:
 * - Device not found
 * - Device busy
 * - Device initialization failure
 * - Device format not supported
 */
class device_error : public musac_error {
public:
    using musac_error::musac_error;
};

/**
 * @brief Audio format related errors
 * 
 * Thrown when format operations fail, such as:
 * - Unsupported audio format
 * - Invalid format parameters
 * - Format conversion failure
 */
class format_error : public musac_error {
public:
    using musac_error::musac_error;
};

/**
 * @brief Decoder related errors
 * 
 * Thrown when decoder operations fail, such as:
 * - Invalid file format
 * - Corrupted data
 * - Unsupported codec features
 * - Decoder initialization failure
 */
class decoder_error : public musac_error {
public:
    using musac_error::musac_error;
};

/**
 * @brief Codec specific errors
 * 
 * Thrown by specific codec implementations for codec-specific issues.
 * This is more specific than decoder_error and indicates a problem
 * with a particular codec's data or state.
 */
class codec_error : public musac_error {
public:
    using musac_error::musac_error;
};

/**
 * @brief I/O stream related errors
 * 
 * Thrown when I/O operations fail, such as:
 * - File not found
 * - Read/write errors
 * - Seek failures
 * - Stream closed unexpectedly
 */
class io_error : public musac_error {
public:
    using musac_error::musac_error;
};

/**
 * @brief Resource related errors
 * 
 * Thrown when resource allocation fails, such as:
 * - Out of memory
 * - Too many open devices
 * - System resource exhaustion
 */
class resource_error : public musac_error {
public:
    using musac_error::musac_error;
};

/**
 * @brief State related errors
 * 
 * Thrown when operations are attempted in invalid states, such as:
 * - Playing a closed stream
 * - Stopping an already stopped stream
 * - Operating on uninitialized objects
 */
class state_error : public musac_error {
public:
    using musac_error::musac_error;
};

} // namespace musac

/*
 * Copyright (C) 2025
 *
 * This file is part of musac.
 *
 * musac is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option) any
 * later version.
 *
 * musac is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with musac.  If not, see <http://www.gnu.org/licenses/>.
 */