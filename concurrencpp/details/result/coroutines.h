#ifndef CONCURRENCPP_COROUTINES_H
#define CONCURRENCPP_COROUTINES_H

#include "result.h"

namespace concurrencpp::details {
	template<class type>
	class final_awaiter {

	private:
		result_state<type>& m_state;

	public:
		final_awaiter(result_state<type>& state) noexcept : m_state(state) {}

		bool await_ready() const noexcept {
			return m_state.dec_ref_coroutine_step_1();
		}

		void await_suspend(std::experimental::coroutine_handle<void> destroy_handle) noexcept {
			m_state.dec_ref_coroutine_step_2(destroy_handle);
		}

		static void await_resume() noexcept {}
	};

	template<class type>
	class coroutine_adapter_base {

	protected:
		result_state<type> m_state;

	public:
		::concurrencpp::result<type> get_return_object() noexcept { return m_state.template share_state_as<result<type>>(); }
		void unhandled_exception() noexcept { m_state.set_exception(std::current_exception()); }
		auto initial_suspend() const noexcept { return std::experimental::suspend_never{}; }
		auto final_suspend() noexcept { return final_awaiter<type>(m_state); }
	};

	template<class type>
	struct coroutine_adapter : public coroutine_adapter_base<type> {
		template<class return_type>
		void return_value(return_type&& value) { this->m_state.set_result(std::forward<return_type>(value)); }
	};

	template<>
	struct coroutine_adapter<void> : public coroutine_adapter_base<void> {
		void return_void() { this->m_state.set_result(); }
	};
}

namespace std::experimental {
	template<class type, class... arguments>
	struct coroutine_traits<::concurrencpp::result<type>, arguments...> {
		using promise_type = concurrencpp::details::coroutine_adapter<type>;
	};
}

#endif //CONCURRENCPP_COROUTINES_H