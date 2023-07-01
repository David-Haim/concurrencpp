#ifndef CONCURRENCPP_SHARED_RESULT_H
#define CONCURRENCPP_SHARED_RESULT_H

#include "concurrencpp/results/result.h"
#include "concurrencpp/results/shared_result_awaitable.h"
#include "concurrencpp/results/impl/shared_result_state.h"

namespace concurrencpp {
    template<class type>
    class shared_result {

       private:
        std::shared_ptr<details::shared_result_state<type>> m_state;

        void throw_if_empty(const char* message) const {
            if (!static_cast<bool>(m_state)) {
                throw errors::empty_result(message);
            }
        }

       public:
        shared_result() noexcept = default;
        ~shared_result() noexcept = default;

        shared_result(std::shared_ptr<details::shared_result_state<type>> state) noexcept : m_state(std::move(state)) {}

        shared_result(result<type> rhs) {
            if (!static_cast<bool>(rhs)) {
                return;
            }

            auto result_state = details::shared_result_helper::get_state(rhs);
            m_state = std::make_shared<details::shared_result_state<type>>(std::move(result_state));
            m_state->share(std::static_pointer_cast<details::shared_result_state_base>(m_state));
        }

        shared_result(const shared_result& rhs) noexcept = default;
        shared_result(shared_result&& rhs) noexcept = default;

        shared_result& operator=(const shared_result& rhs) noexcept {
            if (this != &rhs && m_state != rhs.m_state) {
                m_state = rhs.m_state;
            }

            return *this;
        }

        shared_result& operator=(shared_result&& rhs) noexcept {
            if (this != &rhs && m_state != rhs.m_state) {
                m_state = std::move(rhs.m_state);
            }

            return *this;
        }

        operator bool() const noexcept {
            return static_cast<bool>(m_state.get());
        }

        result_status status() const {
            throw_if_empty(details::consts::k_shared_result_status_error_msg);
            return m_state->status();
        }

        void wait() {
            throw_if_empty(details::consts::k_shared_result_wait_error_msg);
            m_state->wait();
        }

        template<class duration_type, class ratio_type>
        result_status wait_for(std::chrono::duration<duration_type, ratio_type> duration) {
            throw_if_empty(details::consts::k_shared_result_wait_for_error_msg);
            return m_state->wait_for(duration);
        }

        template<class clock_type, class duration_type>
        result_status wait_until(std::chrono::time_point<clock_type, duration_type> timeout_time) {
            throw_if_empty(details::consts::k_shared_result_wait_until_error_msg);
            return m_state->wait_until(timeout_time);
        }

        std::add_lvalue_reference_t<type> get() {
            throw_if_empty(details::consts::k_shared_result_get_error_msg);
            m_state->wait();
            return m_state->get();
        }

        auto operator co_await() {
            throw_if_empty(details::consts::k_shared_result_operator_co_await_error_msg);
            return shared_awaitable<type> {m_state};
        }

        auto resolve() {
            throw_if_empty(details::consts::k_shared_result_resolve_error_msg);
            return shared_resolve_awaitable<type> {m_state};
        }
    };
}  // namespace concurrencpp

#endif
