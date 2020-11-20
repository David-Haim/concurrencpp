#ifndef CONCURRENCPP_RESULT_AWAITABLE_H
#define CONCURRENCPP_RESULT_AWAITABLE_H

#include "concurrencpp/results/result_fwd_declerations.h"

namespace concurrencpp {
    template<class type>
    class awaitable : public std::experimental::suspend_always {

       private:
        details::await_context m_await_ctx;
        std::shared_ptr<details::result_state<type>> m_state;

       public:
        awaitable(std::shared_ptr<details::result_state<type>> state) noexcept : m_state(std::move(state)) {}

        awaitable(const awaitable& rhs) noexcept = delete;
        awaitable(awaitable&& rhs) noexcept = delete;

        bool await_suspend(std::experimental::coroutine_handle<> caller_handle) noexcept {
            assert(static_cast<bool>(m_state));
            m_await_ctx.set_coro_handle(caller_handle);
            return m_state->await(m_await_ctx);
        }

        type await_resume() {
            auto state = std::move(m_state);
            m_await_ctx.throw_if_interrupted();
            return state->get();
        }
    };

    template<class type>
    class via_awaitable : public std::experimental::suspend_always {

       private:
        details::await_via_context m_await_context;
        std::shared_ptr<details::result_state<type>> m_state;
        const bool m_force_rescheduling;

       public:
        via_awaitable(std::shared_ptr<details::result_state<type>> state, std::shared_ptr<concurrencpp::executor> executor, bool force_rescheduling) noexcept :
            m_await_context(std::move(executor)), m_state(std::move(state)), m_force_rescheduling(force_rescheduling) {}

        via_awaitable(const via_awaitable& rhs) noexcept = delete;
        via_awaitable(via_awaitable&& rhs) noexcept = delete;

        bool await_suspend(std::experimental::coroutine_handle<> caller_handle) {
            assert(static_cast<bool>(m_state));

            m_await_context.set_coro_handle(caller_handle);
            return m_state->await_via(m_await_context, m_force_rescheduling);
        }

        type await_resume() {
            auto state = std::move(m_state);
            m_await_context.throw_if_interrupted();
            return state->get();
        }
    };

    template<class type>
    class resolve_awaitable final : public std::experimental::suspend_always {

       private:
        details::await_context m_await_ctx;
        std::shared_ptr<details::result_state<type>> m_state;

       public:
        resolve_awaitable(std::shared_ptr<details::result_state<type>> state) noexcept : m_state(std::move(state)) {}

        resolve_awaitable(resolve_awaitable&&) noexcept = delete;
        resolve_awaitable(const resolve_awaitable&) noexcept = delete;

        bool await_suspend(std::experimental::coroutine_handle<> caller_handle) noexcept {
            assert(static_cast<bool>(m_state));
            m_await_ctx.set_coro_handle(caller_handle);
            return m_state->await(m_await_ctx);
        }

        result<type> await_resume() {
            auto state = std::move(m_state);
            m_await_ctx.throw_if_interrupted();
            return result<type>(std::move(state));
        }
    };

    template<class type>
    class resolve_via_awaitable final : public std::experimental::suspend_always {

       private:
        details::await_via_context m_await_context;
        std::shared_ptr<details::result_state<type>> m_state;
        const bool m_force_rescheduling;

       public:
        resolve_via_awaitable(std::shared_ptr<details::result_state<type>> state, std::shared_ptr<concurrencpp::executor> executor, bool force_rescheduling) noexcept
            :
            m_await_context(std::move(executor)),
            m_state(state), m_force_rescheduling(force_rescheduling) {}

        resolve_via_awaitable(const resolve_via_awaitable&) noexcept = delete;
        resolve_via_awaitable(resolve_via_awaitable&&) noexcept = delete;

        bool await_suspend(std::experimental::coroutine_handle<> caller_handle) {
            assert(static_cast<bool>(m_state));

            m_await_context.set_coro_handle(caller_handle);
            return m_state->await_via(m_await_context, m_force_rescheduling);
        }

        result<type> await_resume() {
            auto state = std::move(m_state);
            m_await_context.throw_if_interrupted();
            return result<type>(std::move(state));
        }
    };
}  // namespace concurrencpp

#endif