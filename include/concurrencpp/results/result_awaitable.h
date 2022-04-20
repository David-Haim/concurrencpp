#ifndef CONCURRENCPP_RESULT_AWAITABLE_H
#define CONCURRENCPP_RESULT_AWAITABLE_H

#include "concurrencpp/coroutines/coroutine.h"
#include "concurrencpp/results/impl/result_state.h"

namespace concurrencpp::details {
    template<class type>
    class awaitable_base : public suspend_always {
       protected:
        consumer_result_state_ptr<type> m_state;

       public:
        awaitable_base(consumer_result_state_ptr<type> state) noexcept : m_state(std::move(state)) {}

        awaitable_base(const awaitable_base&) = delete;
        awaitable_base(awaitable_base&&) = delete;
    };
}  // namespace concurrencpp::details

namespace concurrencpp {
    template<class type>
    class awaitable : public details::awaitable_base<type> {

       public:
        awaitable(details::consumer_result_state_ptr<type> state) noexcept : details::awaitable_base<type>(std::move(state)) {}

        bool await_suspend(details::coroutine_handle<void> caller_handle) noexcept {
            assert(static_cast<bool>(this->m_state));
            return this->m_state->await(caller_handle);
        }

        type await_resume() {
            details::joined_consumer_result_state_ptr<type> state(this->m_state.release());
            return state->get();
        }
    };

    template<class type>
    class resolve_awaitable : public details::awaitable_base<type> {

       public:
        resolve_awaitable(details::consumer_result_state_ptr<type> state) noexcept : details::awaitable_base<type>(std::move(state)) {}

        resolve_awaitable(resolve_awaitable&&) noexcept = delete;
        resolve_awaitable(const resolve_awaitable&) noexcept = delete;

        bool await_suspend(details::coroutine_handle<void> caller_handle) noexcept {
            assert(static_cast<bool>(this->m_state));
            return this->m_state->await(caller_handle);
        }

        result<type> await_resume() {
            return result<type>(std::move(this->m_state));
        }
    };
}  // namespace concurrencpp

#endif