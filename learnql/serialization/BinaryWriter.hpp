#ifndef LEARNQL_SERIALIZATION_BINARY_WRITER_HPP
#define LEARNQL_SERIALIZATION_BINARY_WRITER_HPP

#include "../concepts/Serializable.hpp"
#include <vector>
#include <cstring>
#include <span>
#include <bit>

namespace learnql::serialization {

// Forward declaration
class BinaryReader;

/**
 * @brief Type-safe binary writer using C++20 concepts
 * @details Provides methods to serialize different types to a binary buffer
 *
 * Features:
 * - Concept-based type safety
 * - Automatic endianness handling
 * - Support for primitives, strings, containers
 * - RAII buffer management
 *
 * Example:
 * @code
 * BinaryWriter writer;
 * writer.write(42);
 * writer.write(std::string("Hello"));
 * writer.write(3.14);
 * auto data = writer.get_buffer();
 * @endcode
 */
class BinaryWriter {
public:
    /**
     * @brief Constructs a writer with optional initial capacity
     * @param initial_capacity Initial buffer capacity in bytes
     */
    explicit BinaryWriter(std::size_t initial_capacity = 1024)
        : buffer_{} {
        buffer_.reserve(initial_capacity);
    }

    /**
     * @brief Writes an arithmetic type (int, float, etc.)
     * @tparam T Arithmetic type
     * @param value Value to write
     * @requires T must satisfy Arithmetic concept
     */
    template<concepts::Arithmetic T>
    void write(T value) {
        // For educational purposes: demonstrate endianness handling
        // In production, you might want to always use little-endian
        if constexpr (std::endian::native == std::endian::little) {
            write_raw(&value, sizeof(T));
        } else {
            // Convert to little-endian if needed
            T converted = value;
            std::reverse(reinterpret_cast<uint8_t*>(&converted),
                        reinterpret_cast<uint8_t*>(&converted) + sizeof(T));
            write_raw(&converted, sizeof(T));
        }
    }

    /**
     * @brief Writes a string
     * @tparam T String-like type
     * @param str String to write
     * @requires T must satisfy StringLike concept
     * @details Writes length (as uint32_t) followed by string data
     */
    template<concepts::StringLike T>
    void write(const T& str) {
        const std::string& s = str;
        // Write length first
        auto length = static_cast<uint32_t>(s.size());
        write(length);
        // Write string data
        if (length > 0) {
            write_raw(s.data(), length);
        }
    }

    /**
     * @brief Writes a container (vector, list, etc.)
     * @tparam Container Container type
     * @param container Container to write
     * @requires Container must satisfy Container concept but not StringLike
     * @details Writes size followed by each element
     */
    template<concepts::Container Container>
    requires (!concepts::StringLike<Container>)
    void write(const Container& container) {
        // Write container size
        auto size = static_cast<uint32_t>(container.size());
        write(size);
        // Write each element
        for (const auto& element : container) {
            write(element);
        }
    }

    /**
     * @brief Writes a type with custom serialization
     * @tparam T Type with custom serialization
     * @param obj Object to write
     * @requires T must have serialize(BinaryWriter&) method
     */
    template<typename T>
    requires concepts::CustomSerializable<T, BinaryWriter, BinaryReader>
    void write(const T& obj) {
        obj.serialize(*this);
    }

    /**
     * @brief Writes a smart pointer
     * @tparam T Smart pointer type
     * @param ptr Smart pointer to write
     * @requires T must satisfy SmartPointer concept
     * @details Writes null flag, then data if not null
     */
    template<concepts::SmartPointer T>
    void write(const T& ptr) {
        bool is_null = (ptr == nullptr);
        write(is_null);
        if (!is_null) {
            write(*ptr);
        }
    }

    /**
     * @brief Writes raw bytes to the buffer
     * @param data Pointer to data
     * @param size Number of bytes to write
     */
    void write_raw(const void* data, std::size_t size) {
        const auto* bytes = static_cast<const uint8_t*>(data);
        buffer_.insert(buffer_.end(), bytes, bytes + size);
    }

    /**
     * @brief Writes a span of bytes
     * @param data Span of bytes to write
     */
    void write_bytes(std::span<const uint8_t> data) {
        buffer_.insert(buffer_.end(), data.begin(), data.end());
    }

    /**
     * @brief Gets the current buffer
     * @return Span of the buffer
     */
    [[nodiscard]] std::span<const uint8_t> get_buffer() const noexcept {
        return std::span<const uint8_t>(buffer_.data(), buffer_.size());
    }

    /**
     * @brief Gets the buffer as a vector (for moving)
     * @return Vector containing the buffer
     */
    [[nodiscard]] std::vector<uint8_t> take_buffer() noexcept {
        return std::move(buffer_);
    }

    /**
     * @brief Gets the current size of the buffer
     * @return Size in bytes
     */
    [[nodiscard]] std::size_t size() const noexcept {
        return buffer_.size();
    }

    /**
     * @brief Clears the buffer
     */
    void clear() noexcept {
        buffer_.clear();
    }

    /**
     * @brief Resets the buffer with a new capacity
     * @param capacity New capacity
     */
    void reset(std::size_t capacity = 1024) {
        buffer_.clear();
        buffer_.reserve(capacity);
    }

    /**
     * @brief Gets the current write position (same as size)
     * @return Current position in bytes
     */
    [[nodiscard]] std::size_t position() const noexcept {
        return buffer_.size();
    }

private:
    std::vector<uint8_t> buffer_; ///< Internal buffer
};

} // namespace learnql::serialization

#endif // LEARNQL_SERIALIZATION_BINARY_WRITER_HPP
