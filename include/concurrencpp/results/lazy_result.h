#ifndef CONCURRENCPP_LAZY_RESULT_H
#define CONCURRENCPP_LAZY_RESULT_H

#include "concurrencpp/results/promises.h"
#include "concurrencpp/results/constants.h"
#include "concurrencpp/results/lazy_result_awaitable.h"
#include "concurrencpp/results/impl/lazy_result_state.h"

namespace concurrencpp {
    template<class type>
    class lazy_result {

       private:
        details::coroutine_handle<details::lazy_result_state<type>> m_state;

        void throw_if_empty(const char* err_msg) const {
            if (!static_cast<bool>(m_state)) {
                throw errors::empty_result(err_msg);
            }
        }

        result<type> run_impl() {
            lazy_result self(std::move(*this));
            co_return co_await self;
        }

       public:
        lazy_result() noexcept = default;

        lazy_result(lazy_result&& rhs) noexcept : m_state(std::exchange(rhs.m_state, {})) {}

        lazy_result(details::coroutine_handle<details::lazy_result_state<type>> state) noexcept : m_state(state) {}

        ~lazy_result() noexcept {
            if (static_cast<bool>(m_state)) {
                m_state.destroy();
            }
        }

        lazy_result& operator=(lazy_result&& rhs) noexcept {
            if (&rhs == this) {
                return *this;
            }

            if (static_cast<bool>(m_state)) {
                m_state.destroy();
            }

            m_state = std::exchange(rhs.m_state, {});
            return *this;
        }

        explicit operator bool() const noexcept {
            return static_cast<bool>(m_state);
        }

        result_status status() const {
            throw_if_empty(details::consts::k_empty_lazy_result_status_err_msg);
            return m_state.promise().status();
        }

        auto operator co_await() {
            throw_if_empty(details::consts::k_empty_lazy_result_operator_co_await_err_msg);
            return lazy_awaitable<type> {std::exchange(m_state, {})};
        }

        auto resolve() {
            throw_if_empty(details::consts::k_empty_lazy_result_resolve_err_msg);
            return lazy_resolve_awaitable<type> {std::exchange(m_state, {})};
        }

        result<type> run() {
            throw_if_empty(details::consts::k_empty_lazy_result_run_err_msg);
            return run_impl();
        }
    };
}  // namespace concurrencpp

#endif