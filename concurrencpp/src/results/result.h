#ifndef CONCURRENCPP_RESULT_H
#define CONCURRENCPP_RESULT_H

#include "constants.h"
#include "promises.h"
#include "result_awaitable.h"

#include "../errors.h"
#include "../forward_declerations.h"

#include "../utils/bind.h"

#include <type_traits>

namespace concurrencpp {
	template <class type>
	class result {

		static constexpr auto valid_result_type_v =
			std::is_same_v<type, void> || std::is_nothrow_move_constructible_v<type>;

		static_assert(valid_result_type_v,
			"concurrencpp::result<type> - <<type>> should be now-throw-move constructable or void.");

	private:
		std::shared_ptr<details::result_core<type>> m_state;

		void throw_if_empty(const char* message) const {
			if (m_state.get() != nullptr) {
				return;
			}

			throw errors::empty_result(message);
		}

	public:
		result() noexcept = default;
		~result() noexcept = default;
		result(result&& rhs) noexcept = default;

		result(std::shared_ptr<details::result_core<type>> state) noexcept : m_state(std::move(state)) {}

		result& operator = (result&& rhs) noexcept {
			if (this != &rhs) {
				m_state = std::move(rhs.m_state);
			}

			return *this;
		}

		result(const result& rhs) = delete;
		result& operator = (const result& rhs) = delete;

		operator bool() const noexcept {
			return m_state.get() != nullptr;
		}

		result_status status() const {
			throw_if_empty(details::consts::k_result_status_error_msg);
			return m_state->status();
		}

		void wait() {
			throw_if_empty(details::consts::k_result_wait_error_msg);
			m_state->wait();
		}

		template<class duration_unit, class ratio>
		result_status wait_for(std::chrono::duration<duration_unit, ratio> duration) {
			throw_if_empty(details::consts::k_result_wait_for_error_msg);
			return m_state->wait_for(duration);
		}

		template< class clock, class duration >
		result_status wait_until(std::chrono::time_point<clock, duration> timeout_time) {
			throw_if_empty(details::consts::k_result_wait_until_error_msg);
			return m_state->wait_until(timeout_time);
		}

		type get() {
			throw_if_empty(details::consts::k_result_get_error_msg);
			auto state = std::move(m_state);
			state->wait();
			return state->get();
		}

		auto operator co_await() {
			throw_if_empty(details::consts::k_result_operator_co_await_error_msg);
			return awaitable<type>{ std::move(m_state) };
		}

		auto await_via(
			std::shared_ptr<concurrencpp::executor> executor,
			bool force_rescheduling = true) {
			throw_if_empty(details::consts::k_result_await_via_error_msg);
			return via_awaitable<type>{ std::move(m_state), std::move(executor), force_rescheduling };
		}

		auto resolve() {
			throw_if_empty(details::consts::k_result_resolve_error_msg);
			return resolve_awaitable<type>{ std::move(m_state) };
		}

		auto resolve_via(
			std::shared_ptr<concurrencpp::executor> executor,
			bool force_rescheduling = true) {
			throw_if_empty(details::consts::k_result_resolve_via_error_msg);
			return resolve_via_awaitable<type>{ std::move(m_state), std::move(executor), force_rescheduling };
		}
	};
}

namespace concurrencpp {
	template <class type>
	class result_promise {

	private:
		std::shared_ptr<details::result_core<type>> m_state;
		bool m_result_retrieved;

		void throw_if_empty(const char* message) const {
			if (static_cast<bool>(m_state)) {
				return;
			}

			throw errors::empty_result_promise(message);
		}

		void break_task_if_needed() noexcept {
			if (!static_cast<bool>(m_state)) {
				return;
			}

			if (!m_result_retrieved) { //no result to break.
				return;
			}

			auto exception_ptr =
				std::make_exception_ptr(errors::broken_task("result_promise - broken task."));
			m_state->set_exception(exception_ptr);
			m_state->publish_result();
		}

	public:
		result_promise() :
			m_state(std::make_shared<details::result_core<type>>()),
			m_result_retrieved(false){}

		result_promise(result_promise&& rhs) noexcept :
			m_state(std::move(rhs.m_state)),
			m_result_retrieved(rhs.m_result_retrieved) {}

		~result_promise() noexcept {
			break_task_if_needed();
		}

		result_promise& operator = (result_promise&& rhs) noexcept {
			if (this != &rhs) {
				break_task_if_needed();
				m_state = std::move(rhs.m_state);
				m_result_retrieved = rhs.m_result_retrieved;
			}

			return *this;
		}

		explicit operator bool() const noexcept { return static_cast<bool>(m_state); }

		template<class ... argument_types>
		void set_result(argument_types&& ... arguments) {
			constexpr auto is_constructable =
				std::is_constructible_v<type, argument_types...> || std::is_same_v<void, type>;
			static_assert(is_constructable,
				"result_promise::set_result() - <<type>> is not constructable from <<arguments...>>");

			throw_if_empty(details::consts::k_result_promise_set_result_error_msg);

			m_state->set_result(std::forward<argument_types>(arguments)...);
			m_state->publish_result();
			m_state.reset();
		}

		void set_exception(std::exception_ptr exception_ptr) {
			throw_if_empty(details::consts::k_result_promise_set_exception_error_msg);

			if (!static_cast<bool>(exception_ptr)) {
				throw std::invalid_argument(details::consts::k_result_promise_set_exception_null_exception_error_msg);
			}

			m_state->set_exception(exception_ptr);
			m_state->publish_result();
			m_state.reset();
		}

		template<class callable_type, class ... argument_types>
		void set_from_function(callable_type&& callable, argument_types&& ... args) {
			constexpr auto is_invokable = std::is_invocable_r_v<type, callable_type, argument_types...>;

			static_assert(is_invokable,
				"result_promise::set_from_function() - function(args...) is not invokable or its return type can't be used to construct <<type>>");

			throw_if_empty(details::consts::k_result_promise_set_from_function_error_msg);
			m_state->from_callable(
				details::bind(std::forward<callable_type>(callable), std::forward<argument_types>(args)...));
			m_state->publish_result();
			m_state.reset();
		}

		result<type> get_result() {
			throw_if_empty(details::consts::k_result_get_error_msg);

			if (m_result_retrieved) {
				throw errors::result_already_retrieved(details::consts::k_result_promise_get_result_already_retrieved_error_msg);
			}

			m_result_retrieved = true;
			return result<type>(m_state);
		}
	};

	template<class type, class ... argument_types>
	result<type> make_ready_result(argument_types&& ... arguments) {
		result_promise<type> promise;
		promise.set_result(std::forward<argument_types>(arguments)...);
		return promise.get_result();
	}

	template<class type>
	result<type> make_exceptional_result(std::exception_ptr exception_ptr) {
		result_promise<type> promise;
		promise.set_exception(exception_ptr);
		return promise.get_result();
	}

	template<class type, class exception_type>
	result<type> make_exceptional_result(exception_type exception) {
		return make_exceptional_result(std::make_exception_ptr(exception));
	}
}

#endif