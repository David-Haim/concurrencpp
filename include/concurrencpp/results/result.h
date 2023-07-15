#ifndef CONCURRENCPP_RESULT_H
#define CONCURRENCPP_RESULT_H

#include "concurrencpp/errors.h"
#include "concurrencpp/utils/bind.h"
#include "concurrencpp/results/constants.h"
#include "concurrencpp/results/result_awaitable.h"
#include "concurrencpp/results/impl/result_state.h"

#include <type_traits>

namespace concurrencpp {
    template<class type>
    class result {

        static constexpr auto valid_result_type_v = std::is_same_v<type, void> || std::is_nothrow_move_constructible_v<type>;

        static_assert(valid_result_type_v, "concurrencpp::result<type> - <<type>> should be no-throw-move constructible or void.");

        friend class details::when_result_helper;
        friend struct details::shared_result_helper;

       private:
        details::consumer_result_state_ptr<type> m_state;

        void throw_if_empty(const char* message) const {
            if (static_cast<bool>(!m_state)) {
                throw errors::empty_result(message);
            }
        }

       public:
        result() noexcept = default;
        result(result&& rhs) noexcept = default;

        result(details::consumer_result_state_ptr<type> state) noexcept : m_state(std::move(state)) {}
        result(details::result_state<type>* state) noexcept : m_state(state) {}

        result& operator=(result&& rhs) noexcept {
            if (this != &rhs) {
                m_state = std::move(rhs.m_state);
            }

            return *this;
        }

        result(const result& rhs) = delete;
        result& operator=(const result& rhs) = delete;

        explicit operator bool() const noexcept {
            return static_cast<bool>(m_state);
        }

        result_status status() const {
            throw_if_empty(details::consts::k_result_status_error_msg);
            return m_state->status();
        }

        void wait() const {
            throw_if_empty(details::consts::k_result_wait_error_msg);
            m_state->wait();
        }

        template<class duration_type, class ratio_type>
        result_status wait_for(std::chrono::duration<duration_type, ratio_type> duration) const {
            throw_if_empty(details::consts::k_result_wait_for_error_msg);
            return m_state->wait_for(duration);
        }

        template<class clock_type, class duration_type>
        result_status wait_until(std::chrono::time_point<clock_type, duration_type> timeout_time) const {
            throw_if_empty(details::consts::k_result_wait_until_error_msg);
            return m_state->wait_until(timeout_time);
        }

        type get() {
            throw_if_empty(details::consts::k_result_get_error_msg);
            m_state->wait();

            details::joined_consumer_result_state_ptr<type> state(m_state.release());
            return state->get();
        }

        auto operator co_await() {
            throw_if_empty(details::consts::k_result_operator_co_await_error_msg);
            return awaitable<type> {std::move(m_state)};
        }

        auto resolve() {
            throw_if_empty(details::consts::k_result_resolve_error_msg);
            return resolve_awaitable<type> {std::move(m_state)};
        }
    };
}  // namespace concurrencpp

namespace concurrencpp {
    template<class type>
    class result_promise {

        static constexpr auto valid_result_type_v = std::is_same_v<type, void> || std::is_nothrow_move_constructible_v<type>;

        static_assert(valid_result_type_v,
                      "concurrencpp::result_promise<type> - <<type>> should be no-throw-move constructible or void.");

       private:
        details::producer_result_state_ptr<type> m_producer_state;
        details::consumer_result_state_ptr<type> m_consumer_state;

        void throw_if_empty(const char* message) const {
            if (!static_cast<bool>(m_producer_state)) {
                throw errors::empty_result_promise(message);
            }
        }

        void break_task_if_needed() noexcept {
            if (!static_cast<bool>(m_producer_state)) {
                return;
            }

            if (static_cast<bool>(m_consumer_state)) {  // no result to break.
                return;
            }

            auto exception_ptr = std::make_exception_ptr(errors::broken_task(details::consts::k_broken_task_exception_error_msg));
            m_producer_state->set_exception(exception_ptr);
            m_producer_state.reset();
        }

       public:
        result_promise() {
            m_producer_state.reset(new details::result_state<type>());
            m_consumer_state.reset(m_producer_state.get());
        }

        result_promise(result_promise&& rhs) noexcept :
            m_producer_state(std::move(rhs.m_producer_state)), m_consumer_state(std::move(rhs.m_consumer_state)) {}

        ~result_promise() noexcept {
            break_task_if_needed();
        }

        result_promise& operator=(result_promise&& rhs) noexcept {
            if (this != &rhs) {
                break_task_if_needed();
                m_producer_state = std::move(rhs.m_producer_state);
                m_consumer_state = std::move(rhs.m_consumer_state);
            }

            return *this;
        }

        result_promise(const result_promise&) = delete;
        result_promise& operator=(const result_promise&) = delete;

        explicit operator bool() const noexcept {
            return static_cast<bool>(m_producer_state);
        }

        template<class... argument_types>
        void set_result(argument_types&&... arguments) {
            constexpr auto is_constructable = std::is_constructible_v<type, argument_types...> || std::is_same_v<void, type>;
            static_assert(is_constructable, "result_promise::set_result() - <<type>> is not constructable from <<arguments...>>");

            throw_if_empty(details::consts::k_result_promise_set_result_error_msg);

            m_producer_state->set_result(std::forward<argument_types>(arguments)...);
            m_producer_state.reset();  // publishes the result
        }

        void set_exception(std::exception_ptr exception_ptr) {
            throw_if_empty(details::consts::k_result_promise_set_exception_error_msg);

            if (!static_cast<bool>(exception_ptr)) {
                throw std::invalid_argument(details::consts::k_result_promise_set_exception_null_exception_error_msg);
            }

            m_producer_state->set_exception(exception_ptr);
            m_producer_state.reset();  // publishes the result
        }

        template<class callable_type, class... argument_types>
        void set_from_function(callable_type&& callable, argument_types&&... args) noexcept {
            constexpr auto is_invokable = std::is_invocable_r_v<type, callable_type, argument_types...>;

            static_assert(
                is_invokable,
                "result_promise::set_from_function() - function(args...) is not invokable or its return type can't be used to construct <<type>>");

            throw_if_empty(details::consts::k_result_promise_set_from_function_error_msg);
            m_producer_state->from_callable(
                details::bind(std::forward<callable_type>(callable), std::forward<argument_types>(args)...));
            m_producer_state.reset();  // publishes the result
        }

        result<type> get_result() {
            throw_if_empty(details::consts::k_result_get_error_msg);

            if (!static_cast<bool>(m_consumer_state)) {
                throw errors::result_already_retrieved(details::consts::k_result_promise_get_result_already_retrieved_error_msg);
            }

            return result<type>(std::move(m_consumer_state));
        }
    };
}  // namespace concurrencpp

#endif