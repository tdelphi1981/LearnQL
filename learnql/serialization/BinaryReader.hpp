#ifndef LEARNQL_SERIALIZATION_BINARY_READER_HPP
#define LEARNQL_SERIALIZATION_BINARY_READER_HPP

#include "../concepts/Serializable.hpp"
#include <vector>
#include <cstring>
#include <span>
#include <bit>
#include <stdexcept>

namespace learnql::serialization {

// Forward declaration
class BinaryWriter;

/**
 * @brief Type-safe binary reader using C++20 concepts
 * @details Provides methods to deserialize different types from a binary buffer
 *
 * Features:
 * - Concept-based type safety
 * - Automatic endianness handling
 * - Support for primitives, strings, containers
 * - Bounds checking
 *
 * Example:
 * @code
 * BinaryReader reader(data);
 * int value = reader.read<int>();
 * std::string str = reader.read<std::string>();
 * double pi = reader.read<double>();
 * @endcode
 */
class BinaryReader {
public:
    /**
     * @brief Constructs a reader from a span of bytes
     * @param data Span of bytes to read from
     */
    explicit BinaryReader(std::span<const uint8_t> data)
        : data_{data}, position_{0} {}

    /**
     * @brief Constructs a reader from a vector
     * @param data Vector of bytes to read from
     */
    explicit BinaryReader(const std::vector<uint8_t>& data)
        : data_{std::span<const uint8_t>(data.data(), data.size())}, position_{0} {}

    /**
     * @brief Reads an arithmetic type (int, float, etc.)
     * @tparam T Arithmetic type
     * @return Deserialized value
     * @throws std::runtime_error if not enough data
     * @requires T must satisfy Arithmetic concept
     */
    template<concepts::Arithmetic T>
    [[nodiscard]] T read() {
        T value;
        read_raw(&value, sizeof(T));

        // Handle endianness
        if constexpr (std::endian::native == std::endian::little) {
            return value;
        } else {
            // Convert from little-endian if needed
            std::reverse(reinterpret_cast<uint8_t*>(&value),
                        reinterpret_cast<uint8_t*>(&value) + sizeof(T));
            return value;
        }
    }

    /**
     * @brief Reads a string
     * @return Deserialized string
     * @throws std::runtime_error if not enough data
     * @details Reads length (uint32_t) followed by string data
     */
    [[nodiscard]] std::string read_string() {
        // Read length
        uint32_t length = read<uint32_t>();

        // Validate length
        if (length > remaining()) {
            throw std::runtime_error("String length exceeds available data");
        }

        // Read string data
        std::string result;
        if (length > 0) {
            result.resize(length);
            read_raw(result.data(), length);
        }
        return result;
    }

    /**
     * @brief Reads a container (vector, etc.)
     * @tparam Container Container type to read into
     * @return Deserialized container
     * @throws std::runtime_error if not enough data
     * @requires Container must satisfy Container concept
     */
    template<concepts::Container Container>
    [[nodiscard]] Container read_container() {
        // Read container size
        uint32_t size = read<uint32_t>();

        Container result;
        if constexpr (requires { result.reserve(size); }) {
            result.reserve(size);
        }

        // Read each element
        for (uint32_t i = 0; i < size; ++i) {
            using ValueType = typename Container::value_type;
            if constexpr (concepts::StringLike<ValueType>) {
                result.push_back(read_string());
            } else if constexpr (concepts::CustomSerializable<ValueType, BinaryWriter, BinaryReader>) {
                result.push_back(read_custom<ValueType>());
            } else {
                result.push_back(read<ValueType>());
            }
        }
        return result;
    }

    /**
     * @brief Reads a type with custom deserialization
     * @tparam T Type with custom serialization
     * @return Deserialized object
     * @requires T must have deserialize(BinaryReader&) method
     */
    template<typename T>
    requires concepts::CustomSerializable<T, BinaryWriter, BinaryReader>
    [[nodiscard]] T read_custom() {
        T obj;
        obj.deserialize(*this);
        return obj;
    }

    /**
     * @brief Reads a smart pointer
     * @tparam T Smart pointer type
     * @return Deserialized smart pointer (may be null)
     * @requires T must satisfy SmartPointer concept
     */
    template<concepts::SmartPointer T>
    [[nodiscard]] T read_smart_ptr() {
        bool is_null = read<bool>();
        if (is_null) {
            return T{nullptr};
        }

        using ValueType = typename T::element_type;
        if constexpr (concepts::CustomSerializable<ValueType, BinaryWriter, BinaryReader>) {
            auto value = read_custom<ValueType>();
            return std::make_shared<ValueType>(std::move(value));
        } else if constexpr (concepts::StringLike<ValueType>) {
            auto value = read_string();
            return std::make_shared<ValueType>(std::move(value));
        } else {
            auto value = read<ValueType>();
            return std::make_shared<ValueType>(std::move(value));
        }
    }

    /**
     * @brief Reads raw bytes from the buffer
     * @param dest Destination buffer
     * @param size Number of bytes to read
     * @throws std::runtime_error if not enough data
     */
    void read_raw(void* dest, std::size_t size) {
        if (position_ + size > data_.size()) {
            throw std::runtime_error("Not enough data to read");
        }

        std::memcpy(dest, data_.data() + position_, size);
        position_ += size;
    }

    /**
     * @brief Reads a span of bytes
     * @param size Number of bytes to read
     * @return Span of read bytes (view into original buffer)
     * @throws std::runtime_error if not enough data
     */
    [[nodiscard]] std::span<const uint8_t> read_bytes(std::size_t size) {
        if (position_ + size > data_.size()) {
            throw std::runtime_error("Not enough data to read");
        }

        auto result = data_.subspan(position_, size);
        position_ += size;
        return result;
    }

    /**
     * @brief Gets the current read position
     * @return Current position in bytes
     */
    [[nodiscard]] std::size_t position() const noexcept {
        return position_;
    }

    /**
     * @brief Gets the total size of the buffer
     * @return Size in bytes
     */
    [[nodiscard]] std::size_t size() const noexcept {
        return data_.size();
    }

    /**
     * @brief Gets the number of remaining bytes
     * @return Number of bytes remaining
     */
    [[nodiscard]] std::size_t remaining() const noexcept {
        return data_.size() - position_;
    }

    /**
     * @brief Checks if there are more bytes to read
     * @return true if more data is available
     */
    [[nodiscard]] bool has_more() const noexcept {
        return position_ < data_.size();
    }

    /**
     * @brief Seeks to a specific position
     * @param pos Position to seek to
     * @throws std::out_of_range if position is beyond buffer size
     */
    void seek(std::size_t pos) {
        if (pos > data_.size()) {
            throw std::out_of_range("Seek position out of range");
        }
        position_ = pos;
    }

    /**
     * @brief Resets the read position to the beginning
     */
    void reset() noexcept {
        position_ = 0;
    }

    /**
     * @brief Skips a number of bytes
     * @param count Number of bytes to skip
     * @throws std::runtime_error if skip would exceed buffer size
     */
    void skip(std::size_t count) {
        if (position_ + count > data_.size()) {
            throw std::runtime_error("Skip would exceed buffer size");
        }
        position_ += count;
    }

private:
    std::span<const uint8_t> data_; ///< Data buffer (view)
    std::size_t position_;          ///< Current read position
};

} // namespace learnql::serialization

#endif // LEARNQL_SERIALIZATION_BINARY_READER_HPP
