#ifndef CONCURRENCPP_SHARED_RESULT_AWAITABLE_H
#define CONCURRENCPP_SHARED_RESULT_AWAITABLE_H

#include "concurrencpp/results/impl/shared_result_state.h"

namespace concurrencpp {
    template<class type>
    class shared_awaitable : public details::suspend_always {

       private:
        details::shared_await_context m_await_ctx;
        std::shared_ptr<details::shared_result_state<type>> m_state;

       public:
        shared_awaitable(std::shared_ptr<details::shared_result_state<type>> state) noexcept : m_state(std::move(state)) {}

        shared_awaitable(const shared_awaitable& rhs) noexcept = delete;
        shared_awaitable(shared_awaitable&& rhs) noexcept = delete;

        bool await_suspend(details::coroutine_handle<void> caller_handle) noexcept {
            assert(static_cast<bool>(m_state));
            m_await_ctx.await_context.set_coro_handle(caller_handle);
            return m_state->await(m_await_ctx);
        }

        std::add_lvalue_reference_t<type> await_resume() {
            m_await_ctx.await_context.throw_if_interrupted();
            return m_state->get();
        }
    };

    template<class type>
    class shared_via_awaitable : public details::suspend_always {

       private:
        details::shared_await_via_context m_await_context;
        std::shared_ptr<details::shared_result_state<type>> m_state;
        const bool m_force_rescheduling;

       public:
        shared_via_awaitable(std::shared_ptr<details::shared_result_state<type>> state,
                             std::shared_ptr<concurrencpp::executor> executor,
                             bool force_rescheduling) noexcept :
            m_await_context(std::move(executor)),
            m_state(std::move(state)), m_force_rescheduling(force_rescheduling) {}

        shared_via_awaitable(const shared_via_awaitable& rhs) noexcept = delete;
        shared_via_awaitable(shared_via_awaitable&& rhs) noexcept = delete;

        bool await_suspend(details::coroutine_handle<void> caller_handle) {
            assert(static_cast<bool>(m_state));
            m_await_context.await_context.set_coro_handle(caller_handle);
            return m_state->await_via(m_await_context, m_force_rescheduling);
        }

        std::add_lvalue_reference_t<type> await_resume() {
            m_await_context.await_context.throw_if_interrupted();
            return m_state->get();
        }
    };

    template<class type>
    class shared_resolve_awaitable : public details::suspend_always {

       private:
        details::shared_await_context m_await_ctx;
        std::shared_ptr<details::shared_result_state<type>> m_state;

       public:
        shared_resolve_awaitable(std::shared_ptr<details::shared_result_state<type>> state) noexcept : m_state(std::move(state)) {}

        shared_resolve_awaitable(shared_resolve_awaitable&&) noexcept = delete;
        shared_resolve_awaitable(const shared_resolve_awaitable&) noexcept = delete;

        bool await_suspend(details::coroutine_handle<void> caller_handle) noexcept {
            assert(static_cast<bool>(m_state));
            m_await_ctx.await_context.set_coro_handle(caller_handle);
            return m_state->await(m_await_ctx);
        }

        shared_result<type> await_resume() {
            auto state = std::move(m_state);
            m_await_ctx.await_context.throw_if_interrupted();
            return shared_result<type>(std::move(state));
        }
    };

    template<class type>
    class shared_resolve_via_awaitable : public details::suspend_always {

       private:
        details::shared_await_via_context m_await_context;
        std::shared_ptr<details::shared_result_state<type>> m_state;
        const bool m_force_rescheduling;

       public:
        shared_resolve_via_awaitable(std::shared_ptr<details::shared_result_state<type>> state,
                                     std::shared_ptr<concurrencpp::executor> executor,
                                     bool force_rescheduling) noexcept :
            m_await_context(std::move(executor)),
            m_state(state), m_force_rescheduling(force_rescheduling) {}

        shared_resolve_via_awaitable(const shared_resolve_via_awaitable&) noexcept = delete;
        shared_resolve_via_awaitable(shared_resolve_via_awaitable&&) noexcept = delete;

        bool await_suspend(details::coroutine_handle<void> caller_handle) {
            assert(static_cast<bool>(m_state));

            m_await_context.await_context.set_coro_handle(caller_handle);
            return m_state->await_via(m_await_context, m_force_rescheduling);
        }

        shared_result<type> await_resume() {
            auto state = std::move(m_state);
            m_await_context.await_context.throw_if_interrupted();
            return shared_result<type>(std::move(state));
        }
    };

}  // namespace concurrencpp

#endif
