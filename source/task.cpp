#include "concurrencpp/task.h"
#include "concurrencpp/results/impl/consumer_context.h"

#include <cstring>

using concurrencpp::task;
using concurrencpp::details::vtable;

static_assert(sizeof(task) == concurrencpp::details::task_constants::total_size,
              "concurrencpp::task - object size is bigger than a cache-line.");

using concurrencpp::details::callable_vtable;
using concurrencpp::details::await_via_functor;

namespace concurrencpp::details {
    namespace {
        class coroutine_handle_functor {

           private:
            coroutine_handle<void> m_coro_handle;

           public:
            coroutine_handle_functor() noexcept : m_coro_handle() {}

            coroutine_handle_functor(const coroutine_handle_functor&) = delete;
            coroutine_handle_functor& operator=(const coroutine_handle_functor&) = delete;

            coroutine_handle_functor(coroutine_handle<void> coro_handle) noexcept : m_coro_handle(coro_handle) {}

            coroutine_handle_functor(coroutine_handle_functor&& rhs) noexcept : m_coro_handle(std::exchange(rhs.m_coro_handle, {})) {}

            ~coroutine_handle_functor() noexcept {
                if (static_cast<bool>(m_coro_handle)) {
                    m_coro_handle.destroy();
                }
            }

            void execute_destroy() noexcept {
                auto coro_handle = std::exchange(m_coro_handle, {});
                coro_handle();
            }

            void operator()() noexcept {
                execute_destroy();
            }
        };
    }  // namespace

}  // namespace concurrencpp::details

using concurrencpp::details::coroutine_handle_functor;

void task::build(task&& rhs) noexcept {
    m_vtable = std::exchange(rhs.m_vtable, nullptr);
    if (m_vtable == nullptr) {
        return;
    }

    if (contains<coroutine_handle_functor>(m_vtable)) {
        return callable_vtable<coroutine_handle_functor>::move_destroy(rhs.m_buffer, m_buffer);
    }

    if (contains<await_via_functor>(m_vtable)) {
        return callable_vtable<await_via_functor>::move_destroy(rhs.m_buffer, m_buffer);
    }

    const auto move_destroy_fn = m_vtable->move_destroy_fn;
    if (vtable::trivially_copiable_destructible(move_destroy_fn)) {
        std::memcpy(m_buffer, rhs.m_buffer, details::task_constants::buffer_size);
        return;
    }

    move_destroy_fn(rhs.m_buffer, m_buffer);
}

void task::build(details::coroutine_handle<void> coro_handle) noexcept {
    build(details::coroutine_handle_functor {coro_handle});
}

bool task::contains_coroutine_handle() const noexcept {
    return contains<details::coroutine_handle_functor>();
}

task::task() noexcept : m_buffer(), m_vtable(nullptr) {}

task::task(task&& rhs) noexcept {
    build(std::move(rhs));
}

task::task(details::coroutine_handle<void> coro_handle) noexcept {
    build(coro_handle);
}

task::~task() noexcept {
    clear();
}

void task::operator()() {
    const auto vtable = std::exchange(m_vtable, nullptr);
    if (vtable == nullptr) {
        return;
    }

    if (contains<coroutine_handle_functor>(vtable)) {
        return callable_vtable<coroutine_handle_functor>::execute_destroy(m_buffer);
    }

    if (contains<await_via_functor>(vtable)) {
        return callable_vtable<await_via_functor>::execute_destroy(m_buffer);
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

    if (contains<coroutine_handle_functor>(vtable)) {
        return callable_vtable<coroutine_handle_functor>::destroy(m_buffer);
    }

    if (contains<await_via_functor>(vtable)) {
        return callable_vtable<await_via_functor>::destroy(m_buffer);
    }

    auto destroy_fn = vtable->destroy_fn;
    if (vtable::trivially_destructible(destroy_fn)) {
        return;
    }

    destroy_fn(m_buffer);
}

task::operator bool() const noexcept {
    return m_vtable != nullptr;
}
