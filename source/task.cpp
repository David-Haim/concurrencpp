#include "concurrencpp/task.h"

#include <cstring>

using concurrencpp::task;
using concurrencpp::details::vtable;

static_assert(sizeof(task) == concurrencpp::details::task_constants::total_size, "concurrencpp::task - object size is bigger than a cache-line.");

namespace concurrencpp::details {
    class coroutine_handle_wrapper {

       private:
        coroutine_handle<void> m_coro_handle;

       public:
        coroutine_handle_wrapper(const coroutine_handle_wrapper&) = delete;
        coroutine_handle_wrapper& operator=(const coroutine_handle_wrapper&) = delete;

        coroutine_handle_wrapper(coroutine_handle<void> coro_handle) noexcept : m_coro_handle(coro_handle) {}

        coroutine_handle_wrapper(coroutine_handle_wrapper&& rhs) noexcept : m_coro_handle(rhs.m_coro_handle) {
            rhs.m_coro_handle = {};
        }

        ~coroutine_handle_wrapper() noexcept {
            if (!static_cast<bool>(m_coro_handle)) {
                return;
            }

            m_coro_handle.destroy();
        }

        void execute_destroy() noexcept {
            m_coro_handle();
        }

        void operator()() noexcept {
            execute_destroy();
        }
    };
}  // namespace concurrencpp::details

void task::build(task&& rhs) noexcept {
    m_vtable = rhs.m_vtable;
    if (m_vtable == nullptr) {
        return;
    }

    if (rhs.contains_coro_handle()) {
        build(std::move(rhs.as<details::coroutine_handle_wrapper>()));
        rhs.clear();
        return;
    }

    const auto move_fn = m_vtable->move_fn;
    if (vtable::trivially_copiable(move_fn)) {
        std::memcpy(m_buffer, rhs.m_buffer, details::task_constants::buffer_size);
        rhs.clear();
        return;
    }

    move_fn(rhs.m_buffer, m_buffer);
    rhs.clear();
}

void task::build(details::coroutine_handle<void> coro_handle) noexcept {
    build(details::coroutine_handle_wrapper {coro_handle});
}

task::task() noexcept : m_buffer(), m_vtable(nullptr) {}

task::task(task&& rhs) noexcept {
    build(std::move(rhs));
}

task::~task() noexcept {
    clear();
}

void task::operator()() {
    const auto vtable = std::exchange(m_vtable, nullptr);
    if (vtable == nullptr) {
        return;
    }

    if (contains_coro_handle(vtable)) {
        return as<details::coroutine_handle_wrapper>().execute_destroy();
    }

    vtable->execute_destroy_fn(m_buffer);
}

task& task::operator=(task&& rhs) noexcept {
    if (this == &rhs) {
        return *this;
    }

    clear();
    build(std::move(rhs));
    return *this;
}

void task::clear() noexcept {
    if (m_vtable == nullptr) {
        return;
    }

    const auto vtable = std::exchange(m_vtable, nullptr);
    if (contains_coro_handle(vtable)) {
        as<details::coroutine_handle_wrapper>().~coroutine_handle_wrapper();
        return;
    }

    auto destroy_fn = vtable->destroy_fn;
    if (vtable::trivially_destructable(destroy_fn)) {
        return;
    }

    destroy_fn(m_buffer);
}

task::operator bool() const noexcept {
    return m_vtable != nullptr;
}

bool task::contains_coro_handle(const details::vtable* vtable) noexcept {
    return vtable == &details::callable_vtable<details::coroutine_handle_wrapper>::s_vtable;
}

bool task::contains_coro_handle() const noexcept {
    return contains_coro_handle(m_vtable);
}
