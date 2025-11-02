#ifndef LEARNQL_COROUTINES_ASYNC_QUERY_HPP
#define LEARNQL_COROUTINES_ASYNC_QUERY_HPP

#include "Generator.hpp"
#include "../concepts/Queryable.hpp"
#include <coroutine>
#include <vector>
#include <functional>
#include <memory>

namespace learnql::coroutines {

/**
 * @brief Task for asynchronous operations using coroutines
 * @tparam T Result type
 *
 * Represents an asynchronous operation that can be co_awaited.
 *
 * Example:
 * @code
 * Task<std::vector<Student>> fetch_students() {
 *     co_return students.get_all();
 * }
 *
 * auto task = fetch_students();
 * auto results = co_await task;
 * @endcode
 */
template<typename T = void>
class Task {
public:
    /**
     * @brief Promise type for the Task coroutine
     */
    struct promise_type {
        std::optional<T> result;
        std::exception_ptr exception;

        Task get_return_object() {
            return Task{std::coroutine_handle<promise_type>::from_promise(*this)};
        }

        std::suspend_never initial_suspend() noexcept {
            return {};
        }

        std::suspend_always final_suspend() noexcept {
            return {};
        }

        void return_value(T value) {
            result = std::move(value);
        }

        void unhandled_exception() {
            exception = std::current_exception();
        }
    };

    using handle_type = std::coroutine_handle<promise_type>;

    explicit Task(handle_type h) : coro_(h) {}

    Task(const Task&) = delete;
    Task& operator=(const Task&) = delete;

    Task(Task&& other) noexcept : coro_(other.coro_) {
        other.coro_ = nullptr;
    }

    Task& operator=(Task&& other) noexcept {
        if (this != &other) {
            if (coro_) {
                coro_.destroy();
            }
            coro_ = other.coro_;
            other.coro_ = nullptr;
        }
        return *this;
    }

    ~Task() {
        if (coro_) {
            coro_.destroy();
        }
    }

    /**
     * @brief Awaiter for the task
     */
    struct awaiter {
        handle_type coro;

        bool await_ready() const noexcept {
            return coro.done();
        }

        void await_suspend([[maybe_unused]] std::coroutine_handle<> awaiting) noexcept {
            // In a real async system, we'd schedule the coroutine here
        }

        T await_resume() {
            if (coro.promise().exception) {
                std::rethrow_exception(coro.promise().exception);
            }
            return std::move(*coro.promise().result);
        }
    };

    /**
     * @brief Makes Task awaitable
     */
    awaiter operator co_await() {
        return awaiter{coro_};
    }

    /**
     * @brief Gets the result (blocking)
     */
    T get() {
        if (!coro_.done()) {
            coro_.resume();
        }

        if (coro_.promise().exception) {
            std::rethrow_exception(coro_.promise().exception);
        }

        return std::move(*coro_.promise().result);
    }

    /**
     * @brief Checks if the task is complete
     */
    [[nodiscard]] bool is_ready() const noexcept {
        return coro_ && coro_.done();
    }

private:
    handle_type coro_;
};

/**
 * @brief Specialization for void Task
 */
template<>
class Task<void> {
public:
    struct promise_type {
        std::exception_ptr exception;

        Task get_return_object() {
            return Task{std::coroutine_handle<promise_type>::from_promise(*this)};
        }

        std::suspend_never initial_suspend() noexcept {
            return {};
        }

        std::suspend_always final_suspend() noexcept {
            return {};
        }

        void return_void() noexcept {}

        void unhandled_exception() {
            exception = std::current_exception();
        }
    };

    using handle_type = std::coroutine_handle<promise_type>;

    explicit Task(handle_type h) : coro_(h) {}

    Task(const Task&) = delete;
    Task& operator=(const Task&) = delete;

    Task(Task&& other) noexcept : coro_(other.coro_) {
        other.coro_ = nullptr;
    }

    Task& operator=(Task&& other) noexcept {
        if (this != &other) {
            if (coro_) {
                coro_.destroy();
            }
            coro_ = other.coro_;
            other.coro_ = nullptr;
        }
        return *this;
    }

    ~Task() {
        if (coro_) {
            coro_.destroy();
        }
    }

    struct awaiter {
        handle_type coro;

        bool await_ready() const noexcept {
            return coro.done();
        }

        void await_suspend([[maybe_unused]] std::coroutine_handle<> awaiting) noexcept {
            // In a real async system, we'd schedule the coroutine here
        }

        void await_resume() {
            if (coro.promise().exception) {
                std::rethrow_exception(coro.promise().exception);
            }
        }
    };

    awaiter operator co_await() {
        return awaiter{coro_};
    }

    void get() {
        if (!coro_.done()) {
            coro_.resume();
        }

        if (coro_.promise().exception) {
            std::rethrow_exception(coro_.promise().exception);
        }
    }

    [[nodiscard]] bool is_ready() const noexcept {
        return coro_ && coro_.done();
    }

private:
    handle_type coro_;
};

/**
 * @brief Helper to create a generator that yields records one at a time
 * @tparam T Record type
 * @param records Vector of records
 * @return Generator that yields records
 *
 * Example:
 * @code
 * auto gen = make_generator(students.get_all());
 * for (const auto& student : gen) {
 *     std::cout << student << "\n";
 * }
 * @endcode
 */
template<typename T>
Generator<T> make_generator(std::vector<T> records) {
    for (auto& record : records) {
        co_yield std::move(record);
    }
}

/**
 * @brief Helper to create a generator from a predicate filter
 * @tparam T Record type
 * @tparam Pred Predicate type
 * @param records Vector of records
 * @param pred Predicate function
 * @return Generator that yields matching records
 */
template<typename T, typename Pred>
Generator<T> filter_generator(std::vector<T> records, Pred pred) {
    for (auto& record : records) {
        if (pred(record)) {
            co_yield std::move(record);
        }
    }
}

} // namespace learnql::coroutines

#endif // LEARNQL_COROUTINES_ASYNC_QUERY_HPP
