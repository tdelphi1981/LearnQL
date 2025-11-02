#ifndef LEARNQL_COROUTINES_GENERATOR_HPP
#define LEARNQL_COROUTINES_GENERATOR_HPP

#include <coroutine>
#include <exception>
#include <memory>
#include <optional>

namespace learnql::coroutines {

/**
 * @brief A generator for lazy sequence generation using C++20 coroutines
 * @tparam T The type of values generated
 *
 * This implements a coroutine-based generator that yields values one at a time.
 * It's perfect for lazy evaluation and can significantly reduce memory usage
 * for large datasets.
 *
 * Example:
 * @code
 * Generator<int> fibonacci() {
 *     int a = 0, b = 1;
 *     while (true) {
 *         co_yield a;
 *         auto tmp = a + b;
 *         a = b;
 *         b = tmp;
 *     }
 * }
 *
 * auto fib = fibonacci();
 * for (int i = 0; i < 10; ++i) {
 *     std::cout << fib() << " ";
 * }
 * @endcode
 */
template<typename T>
class Generator {
public:
    /**
     * @brief Promise type for the coroutine
     */
    struct promise_type {
        std::optional<T> current_value;
        std::exception_ptr exception;

        /**
         * @brief Called when coroutine is created
         */
        Generator get_return_object() {
            return Generator{std::coroutine_handle<promise_type>::from_promise(*this)};
        }

        /**
         * @brief Initial suspend - suspend immediately
         */
        std::suspend_always initial_suspend() noexcept {
            return {};
        }

        /**
         * @brief Final suspend - suspend at end
         */
        std::suspend_always final_suspend() noexcept {
            return {};
        }

        /**
         * @brief Handle co_yield expressions
         */
        std::suspend_always yield_value(T value) noexcept {
            current_value = std::move(value);
            return {};
        }

        /**
         * @brief Handle co_return
         */
        void return_void() noexcept {}

        /**
         * @brief Handle exceptions
         */
        void unhandled_exception() {
            exception = std::current_exception();
        }
    };

    using handle_type = std::coroutine_handle<promise_type>;

    /**
     * @brief Constructs a generator from a coroutine handle
     */
    explicit Generator(handle_type h) : coro_(h) {}

    /**
     * @brief Deleted copy constructor
     */
    Generator(const Generator&) = delete;

    /**
     * @brief Move constructor
     */
    Generator(Generator&& other) noexcept
        : coro_(other.coro_) {
        other.coro_ = nullptr;
    }

    /**
     * @brief Deleted copy assignment
     */
    Generator& operator=(const Generator&) = delete;

    /**
     * @brief Move assignment
     */
    Generator& operator=(Generator&& other) noexcept {
        if (this != &other) {
            if (coro_) {
                coro_.destroy();
            }
            coro_ = other.coro_;
            other.coro_ = nullptr;
        }
        return *this;
    }

    /**
     * @brief Destructor - destroys the coroutine
     */
    ~Generator() {
        if (coro_) {
            coro_.destroy();
        }
    }

    /**
     * @brief Advances the generator and returns the next value
     * @return Optional containing the next value, or empty if done
     */
    std::optional<T> next() {
        if (!coro_ || coro_.done()) {
            return std::nullopt;
        }

        coro_.resume();

        if (coro_.promise().exception) {
            std::rethrow_exception(coro_.promise().exception);
        }

        if (coro_.done()) {
            return std::nullopt;
        }

        return std::move(coro_.promise().current_value);
    }

    /**
     * @brief Checks if the generator has more values
     */
    [[nodiscard]] bool has_next() const {
        return coro_ && !coro_.done();
    }

    /**
     * @brief Iterator for the generator
     */
    class Iterator {
    public:
        using iterator_category = std::input_iterator_tag;
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using pointer = const T*;
        using reference = const T&;

        Iterator() = default;

        explicit Iterator(Generator* gen) : gen_(gen) {
            advance();
        }

        reference operator*() const {
            return *current_;
        }

        pointer operator->() const {
            return &(*current_);
        }

        Iterator& operator++() {
            advance();
            return *this;
        }

        Iterator operator++(int) {
            Iterator tmp = *this;
            ++(*this);
            return tmp;
        }

        bool operator==(const Iterator& other) const {
            return gen_ == other.gen_ && (!current_.has_value() == !other.current_.has_value());
        }

        bool operator!=(const Iterator& other) const {
            return !(*this == other);
        }

    private:
        void advance() {
            if (gen_) {
                current_ = gen_->next();
                if (!current_.has_value()) {
                    gen_ = nullptr;
                }
            }
        }

        Generator* gen_ = nullptr;
        std::optional<T> current_;
    };

    /**
     * @brief Returns an iterator to the beginning
     */
    Iterator begin() {
        return Iterator(this);
    }

    /**
     * @brief Returns an iterator to the end
     */
    Iterator end() {
        return Iterator();
    }

private:
    handle_type coro_;
};

} // namespace learnql::coroutines

#endif // LEARNQL_COROUTINES_GENERATOR_HPP
