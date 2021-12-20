#ifndef CONCURRENCPP_LAZY_RESULT_AWAITABLE_H
#define CONCURRENCPP_LAZY_RESULT_AWAITABLE_H

#include "concurrencpp/results/impl/lazy_result_state.h"

namespace concurrencpp {
    template<class type>
    class lazy_awaitable {

       private:
        const details::coroutine_handle<details::lazy_result_state<type>> m_state;

       public:
        lazy_awaitable(details::coroutine_handle<details::lazy_result_state<type>> state) noexcept : m_state(state) {
            assert(static_cast<bool>(state));
        }

        lazy_awaitable(const lazy_awaitable&) = delete;
        lazy_awaitable(lazy_awaitable&&) = delete;

        ~lazy_awaitable() noexcept {
            auto state = m_state;
            state.destroy();
        }

        bool await_ready() const noexcept {
            return m_state.done();
        }

        details::coroutine_handle<void> await_suspend(details::coroutine_handle<void> caller_handle) noexcept {
            return m_state.promise().await(caller_handle);
        }

        type await_resume() {
            return m_state.promise().get();
        }
    };

    template<class type>
    class lazy_resolve_awaitable {

       private:
        details::coroutine_handle<details::lazy_result_state<type>> m_state;

       public:
        lazy_resolve_awaitable(details::coroutine_handle<details::lazy_result_state<type>> state) noexcept : m_state(state) {
            assert(static_cast<bool>(state));
        }

        lazy_resolve_awaitable(const lazy_resolve_awaitable&) = delete;
        lazy_resolve_awaitable(lazy_resolve_awaitable&&) = delete;

        ~lazy_resolve_awaitable() noexcept {
            if (static_cast<bool>(m_state)) {
                m_state.destroy();
            }
        }

        bool await_ready() const noexcept {
            return m_state.done();
        }

        details::coroutine_handle<void> await_suspend(details::coroutine_handle<void> caller_handle) noexcept {
            return m_state.promise().await(caller_handle);
        }

        lazy_result<type> await_resume() noexcept {
            return {std::exchange(m_state, {})};
        }
    };
}  // namespace concurrencpp

#endif