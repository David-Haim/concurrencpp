#ifndef CONCURRENCPP_SHARED_RESULT_AWAITABLE_H
#define CONCURRENCPP_SHARED_RESULT_AWAITABLE_H

#include "concurrencpp/results/impl/shared_result_state.h"

namespace concurrencpp::details {
    template<class type>
    class shared_awaitable_base : public suspend_always {
       protected:
        std::shared_ptr<shared_result_state<type>> m_state;

       public:
        shared_awaitable_base(const std::shared_ptr<shared_result_state<type>>& state) noexcept : m_state(state) {}

        shared_awaitable_base(const shared_awaitable_base&) = delete;
        shared_awaitable_base(shared_awaitable_base&&) = delete;
    };
}  // namespace concurrencpp::details

namespace concurrencpp {
    template<class type>
    class shared_awaitable : public details::shared_awaitable_base<type> {

       private:
        details::shared_await_context m_await_ctx;

       public:
        shared_awaitable(const std::shared_ptr<details::shared_result_state<type>>& state) noexcept :
            details::shared_awaitable_base<type>(state) {}

        bool await_suspend(details::coroutine_handle<void> caller_handle) noexcept {
            assert(static_cast<bool>(this->m_state));
            this->m_await_ctx.caller_handle = caller_handle;
            return this->m_state->await(m_await_ctx);
        }

        std::add_lvalue_reference_t<type> await_resume() {
            return this->m_state->get();
        }
    };

    template<class type>
    class shared_via_awaitable : public details::shared_awaitable_base<type> {

       private:
        details::shared_await_via_context m_await_context;
        const bool m_force_rescheduling;

       public:
        shared_via_awaitable(const std::shared_ptr<details::shared_result_state<type>>& state,
                             const std::shared_ptr<concurrencpp::executor>& executor,
                             bool force_rescheduling) noexcept :
            details::shared_awaitable_base<type>(state),
            m_await_context(executor), m_force_rescheduling(force_rescheduling) {}

        bool await_suspend(details::coroutine_handle<void> caller_handle) {
            assert(static_cast<bool>(this->m_state));
            this->m_await_context.await_context.set_coro_handle(caller_handle);
            return this->m_state->await_via(m_await_context, m_force_rescheduling);
        }

        std::add_lvalue_reference_t<type> await_resume() {
            this->m_await_context.await_context.throw_if_interrupted();
            return this->m_state->get();
        }
    };

    template<class type>
    class shared_resolve_awaitable : public details::shared_awaitable_base<type> {

       private:
        details::shared_await_context m_await_ctx;

       public:
        shared_resolve_awaitable(const std::shared_ptr<details::shared_result_state<type>>& state) noexcept :
            details::shared_awaitable_base<type>(state) {}

        bool await_suspend(details::coroutine_handle<void> caller_handle) noexcept {
            assert(static_cast<bool>(this->m_state));
            this->m_await_ctx.caller_handle = caller_handle;
            return this->m_state->await(m_await_ctx);
        }

        shared_result<type> await_resume() {
            return shared_result<type>(std::move(this->m_state));
        }
    };

    template<class type>
    class shared_resolve_via_awaitable : public details::shared_awaitable_base<type> {

       private:
        details::shared_await_via_context m_await_context;
        const bool m_force_rescheduling;

       public:
        shared_resolve_via_awaitable(const std::shared_ptr<details::shared_result_state<type>>& state,
                                     const std::shared_ptr<concurrencpp::executor>& executor,
                                     bool force_rescheduling) noexcept :
            details::shared_awaitable_base<type>(state),
            m_await_context(executor), m_force_rescheduling(force_rescheduling) {}

        bool await_suspend(details::coroutine_handle<void> caller_handle) {
            assert(static_cast<bool>(this->m_state));
            this->m_await_context.await_context.set_coro_handle(caller_handle);
            return this->m_state->await_via(m_await_context, m_force_rescheduling);
        }

        shared_result<type> await_resume() {
            auto state = std::move(this->m_state);
            this->m_await_context.await_context.throw_if_interrupted();
            return shared_result<type>(std::move(state));
        }
    };
}  // namespace concurrencpp

#endif