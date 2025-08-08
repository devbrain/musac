#pragma once

#include <algorithm>
#include <cstring>
#include <memory>
#include <type_traits>
#include <stdexcept>

namespace musac {
    /*
     * Simple RAII wrapper for buffers/arrays. More restrictive than std::vector.
     */
    template<typename T>
    class buffer final {
        static_assert(std::is_trivially_copyable_v<T>, "buffer<T> requires trivially copyable T");
    public:
        explicit buffer(std::size_t size)
            : m_data(std::make_unique<T[]>(size)), m_size(size) {
            // initialize to default value
            std::fill_n(m_data.get(), m_size, T{});
        }

        [[nodiscard]] std::size_t size() const noexcept {
            return m_size;
        }

        // Returns mutable pointer to data, like std::vector::data()
        T* data() noexcept { return m_data.get(); }
        const T* data() const noexcept { return m_data.get(); }

        // Bounds-checked access
        T& at(std::size_t pos) {
            if (pos >= m_size) throw std::out_of_range("buffer index out of range");
            return m_data[pos];
        }
        const T& at(std::size_t pos) const {
            if (pos >= m_size) throw std::out_of_range("buffer index out of range");
            return m_data[pos];
        }

        // Resets buffer to new size, zero-initializes
        void reset(std::size_t newSize) {
            m_data = std::make_unique<T[]>(newSize);
            m_size = newSize;
            std::fill_n(m_data.get(), m_size, T{});
        }

        // Resize buffer, preserves existing content up to new size
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

        void swap(buffer& other) noexcept {
            m_data.swap(other.m_data);
            std::swap(m_size, other.m_size);
        }

        // unchecked access, noexcept
        T& operator[](std::size_t pos) noexcept { return m_data[pos]; }
        const T& operator[](std::size_t pos) const noexcept { return m_data[pos]; }

        // begin/end, for range-based loops
        T* begin() noexcept { return data(); }
        T* end() noexcept { return data() + size(); }
        const T* begin() const noexcept { return data(); }
        const T* end() const noexcept { return data() + size(); }

    private:
        std::unique_ptr<T[]> m_data;
        std::size_t m_size;
    };
}
