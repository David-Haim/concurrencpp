#ifndef CONCURRENCPP_RESULT_H
#define CONCURRENCPP_RESULT_H

#include "../forward_declerations.h"
#include "../errors.h"

#include "result_fwd_declerations.h"
#include "result_state.h"
#include "result_awaitable.h"

namespace concurrencpp {

	template <class type>
	class result {

	private:
		details::result_state_ptr <type> m_state;

		void throw_if_empty(const char* message) const {
			if (static_cast<bool>(m_state)) {
				return;
			}

			throw concurrencpp::errors::empty_result(message);
		}

		static_assert(
			std::is_same_v<type, void> || std::is_nothrow_move_constructible_v<type>,
			"result<type> - <<type>> should be now-throw-move constructible or void.");

	public:
		result() noexcept = default;
		result(result&& rhs) noexcept = default;

		result(details::result_state_ptr<type> state_ptr) noexcept : m_state(std::move(state_ptr)) {}

		result& operator = (result&& rhs) noexcept = default;

		result(const result& rhs) = delete;
		result& operator = (const result& rhs) = delete;

		result_status status() const {
			throw_if_empty("result::status() - result is empty.");
			return m_state->status();
		}

		inline type get() {
			throw_if_empty("result::get() - result is empty.");
			auto state = std::move(m_state);
			state->wait();
			return state->get();
		}

		void wait() {
			throw_if_empty("result::wait() - result is empty.");
			m_state->wait();
		}

		template<class duration_unit, class ratio>
		result_status wait_for(std::chrono::duration<duration_unit, ratio> duration) {
			throw_if_empty("result::wait_for() - result is empty.");
			return m_state->wait_for(duration);
		}

		template< class clock, class duration >
		result_status wait_until(std::chrono::time_point<clock, duration> timeout_time) const {
			throw_if_empty("result::wait_until() - result is empty.");
			return m_state->wait_until(timeout_time);
		}

		result_awaitable<type> operator co_await() {
			throw_if_empty("result::operator co_await() - result is empty.");
			return result_awaitable<type>(std::move(m_state), nullptr, false);
		}

		result_awaitable<type> await_via(std::shared_ptr<concurrencpp::executor> executor, bool force_rescheduling = true) {
			throw_if_empty("result::await_via() - result is empty.");
			return result_awaitable<type>(std::move(m_state), std::move(executor), force_rescheduling);
		}

		result_resolver<type> resolve() {
			throw_if_empty("result::resolve() - result is empty.");
			return result_resolver<type>(std::move(m_state), nullptr, false);
		}

		result_resolver<type> resolve_via(
			std::shared_ptr<concurrencpp::executor> executor,
			bool force_rescheduling = true) {
			throw_if_empty("result::resolve_via() - result is empty.");
			return result_resolver<type>(std::move(m_state), std::move(executor), force_rescheduling);
		}

		operator bool() const noexcept { return static_cast<bool>(m_state); }
	};
}

namespace concurrencpp {
	template <class type>
	class result_promise {

	private:
		details::result_state_ptr <type> m_state;

		void break_task_if_needed() {
			if (!static_cast<bool>(m_state)) {
				return;
			}

			auto exception_ptr = std::make_exception_ptr(errors::broken_task("result_promise - broken task."));
			m_state->set_exception(exception_ptr);
		}

		template<class function_type, class ... argument_types>
		void set_from_function_impl(std::true_type, function_type&& function, argument_types&& ... args) {
			function(std::forward<argument_types>(args)...);
			set_result();
		}

		template<class function_type, class ... argument_types>
		void set_from_function_impl(std::false_type, function_type&& function, argument_types&& ... args) {
			set_result(function(std::forward<argument_types>(args)...));
		}

		void throw_if_empty(const char* message) {
			if (!static_cast<bool>(m_state)) {
				throw errors::empty_result_promise(message);
			}
		}

	public:
		result_promise() : m_state(new details::result_state<type>()) {}

		result_promise(result_promise&& rhs) noexcept :
			m_state(std::move(rhs.m_state)) {}

		~result_promise() noexcept { break_task_if_needed(); }

		result_promise& operator = (result_promise&& rhs) noexcept {
			if (this != &rhs) {
				break_task_if_needed();
				m_state = std::move(rhs.m_state);
			}

			return *this;
		}

		operator bool() const noexcept { return static_cast<bool>(m_state); }

		template<class ... argument_types>
		void set_result(argument_types&& ... arguments) {
			throw_if_empty("result_promise::set_result() - result_promise is empty.");

			constexpr auto is_constructible = 
				std::is_constructible_v<type, argument_types...> || std::is_same_v<void, type>;
			static_assert(is_constructible,
				"result_promise::set_result() - <<type>> is not constructible from <<args...>>");

			m_state->set_result(std::forward<argument_types>(arguments)...);
			m_state.reset();
		}

		inline void set_exception(std::exception_ptr exception_ptr) {
			throw_if_empty("result_promise::set_exception() - result_promise is empty.");

			if (!static_cast<bool>(exception_ptr)) {
				throw std::invalid_argument("result_promise::set_exception() - exception_ptr is null.");
			}

			m_state->set_exception(exception_ptr);
			m_state.reset();
		}

		template<class function_type, class ... argument_types>
		void set_from_function(function_type&& function, argument_types&& ... args) {
			using is_void_type = typename std::is_same<void, type>::type;
			constexpr auto is_invockable = std::is_invocable_r_v<type, function_type, argument_types...>;

			static_assert(is_invockable,
				"result_promise::set_from_function() - function(args...) is not invocable or its return type can't be used to construct <<type>>");

			throw_if_empty("result_promise::set_from_function() - result_promise is empty.");

			try {
				set_from_function_impl(
					is_void_type(),
					std::forward<function_type>(function),
					std::forward<argument_types>(args)...);
			}
			catch (...) {
				set_exception(std::current_exception());
			}
		}

		inline result<type> get_result() {
			throw_if_empty("result_promise::get_result() - result_promise is empty.");

			if (m_state->result_promise_retreived()) {
				throw errors::result_already_retrieved("result_promise::get_result() - result has already retrieved.");
			}

			m_state->result_promise_retreived(true);
			return m_state->template share_state_as<result<type>>();
		}
	};
}

namespace concurrencpp {

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
