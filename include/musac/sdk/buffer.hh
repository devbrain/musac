/**
 * @file buffer.hh
 * @brief Lightweight buffer container for audio data
 * @ingroup sdk
 */

#pragma once

#include <algorithm>
#include <cstring>
#include <memory>
#include <type_traits>
#include <stdexcept>

namespace musac {
    /**
     * @class buffer
     * @brief RAII buffer container optimized for audio processing
     * @tparam T Element type (must be trivially copyable)
     * @ingroup sdk
     * 
     * A lightweight alternative to std::vector designed specifically for
     * audio buffers. Unlike std::vector, buffer is more restrictive but
     * offers better performance guarantees for real-time audio.
     * 
     * ## Design Rationale
     * 
     * - **No dynamic growth**: Size changes require explicit resize()
     * - **Zero-initialization**: All elements initialized to zero
     * - **Trivially copyable only**: Ensures memcpy optimization
     * - **No capacity tracking**: Size equals capacity always
     * - **RAII memory management**: Automatic cleanup
     * 
     * ## Usage Example
     * 
     * @code
     * // Create a buffer for audio samples
     * buffer<float> audio_buf(1024);
     * 
     * // Fill with audio data
     * for (size_t i = 0; i < audio_buf.size(); ++i) {
     *     audio_buf[i] = generate_sample(i);
     * }
     * 
     * // Resize if needed (preserves data)
     * audio_buf.resize(2048);
     * 
     * // Access raw pointer for C APIs
     * process_audio(audio_buf.data(), audio_buf.size());
     * @endcode
     * 
     * ## Performance Characteristics
     * 
     * - Allocation: O(n) - single allocation
     * - Access: O(1) - direct array access
     * - Resize: O(n) - requires reallocation
     * - No hidden allocations or copies
     * 
     * @note Primarily used for audio sample buffers in the mixer
     */
    template<typename T>
    class buffer final {
        static_assert(std::is_trivially_copyable_v<T>, "buffer<T> requires trivially copyable T");
    public:
        /**
         * @brief Construct buffer with specified size
         * @param size Number of elements
         * 
         * Allocates memory and zero-initializes all elements.
         */
        explicit buffer(std::size_t size)
            : m_data(std::make_unique<T[]>(size)), m_size(size) {
            // initialize to default value
            std::fill_n(m_data.get(), m_size, T{});
        }

        /**
         * @brief Get buffer size
         * @return Number of elements in buffer
         */
        [[nodiscard]] std::size_t size() const noexcept {
            return m_size;
        }

        /**
         * @brief Get pointer to buffer data
         * @return Pointer to first element
         * 
         * Compatible with C APIs that expect raw arrays.
         */
        T* data() noexcept { return m_data.get(); }
        
        /**
         * @brief Get const pointer to buffer data
         * @return Const pointer to first element
         */
        const T* data() const noexcept { return m_data.get(); }

        /**
         * @brief Bounds-checked element access
         * @param pos Element index
         * @return Reference to element
         * @throws std::out_of_range if pos >= size()
         */
        T& at(std::size_t pos) {
            if (pos >= m_size) throw std::out_of_range("buffer index out of range");
            return m_data[pos];
        }
        
        /**
         * @brief Bounds-checked const element access
         * @param pos Element index
         * @return Const reference to element
         * @throws std::out_of_range if pos >= size()
         */
        const T& at(std::size_t pos) const {
            if (pos >= m_size) throw std::out_of_range("buffer index out of range");
            return m_data[pos];
        }

        /**
         * @brief Reset buffer to new size
         * @param newSize New number of elements
         * 
         * Discards all existing data and allocates new zero-initialized buffer.
         * More efficient than resize() when existing data isn't needed.
         */
        void reset(std::size_t newSize) {
            m_data = std::make_unique<T[]>(newSize);
            m_size = newSize;
            std::fill_n(m_data.get(), m_size, T{});
        }

        /**
         * @brief Resize buffer preserving data
         * @param new_size New number of elements
         * 
         * Preserves existing elements up to min(old_size, new_size).
         * New elements are zero-initialized.
         * 
         * @note Requires reallocation and copy
         */
        void resize(std::size_t new_size) {
            auto new_data = std::make_unique<T[]>(new_size);
            // copy existing elements up to min(new_size, old_size)
            if constexpr (std::is_trivially_copyable_v<T>) {
                std::memcpy(new_data.get(), m_data.get(), sizeof(T) * std::min(new_size, m_size));
            } else {
                std::copy_n(m_data.get(), std::min(new_size, m_size), new_data.get());
            }
            // zero-initialize any new elements
            if (new_size > m_size) {
                std::fill(new_data.get() + m_size, new_data.get() + new_size, T{});
            }
            m_data.swap(new_data);
            m_size = new_size;
        }

        /**
         * @brief Swap contents with another buffer
         * @param other Buffer to swap with
         * 
         * Efficient O(1) swap operation.
         */
        void swap(buffer& other) noexcept {
            m_data.swap(other.m_data);
            std::swap(m_size, other.m_size);
        }

        /**
         * @brief Unchecked element access
         * @param pos Element index
         * @return Reference to element
         * 
         * @warning No bounds checking - undefined behavior if pos >= size()
         */
        T& operator[](std::size_t pos) noexcept { return m_data[pos]; }
        
        /**
         * @brief Unchecked const element access
         * @param pos Element index
         * @return Const reference to element
         * 
         * @warning No bounds checking - undefined behavior if pos >= size()
         */
        const T& operator[](std::size_t pos) const noexcept { return m_data[pos]; }

        /**
         * @brief Get iterator to beginning
         * @return Pointer to first element
         */
        T* begin() noexcept { return data(); }
        
        /**
         * @brief Get iterator to end
         * @return Pointer past last element
         */
        T* end() noexcept { return data() + size(); }
        
        /**
         * @brief Get const iterator to beginning
         * @return Const pointer to first element
         */
        const T* begin() const noexcept { return data(); }
        
        /**
         * @brief Get const iterator to end
         * @return Const pointer past last element
         */
        const T* end() const noexcept { return data() + size(); }

    private:
        std::unique_ptr<T[]> m_data;  ///< Buffer data
        std::size_t m_size;            ///< Number of elements
    };
}
