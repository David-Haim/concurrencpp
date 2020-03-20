#ifndef CONCURRENCPP_RESULT_AWAITABLE_H
#define CONCURRENCPP_RESULT_AWAITABLE_H

#include "../errors.h"
#include "result_fwd_declerations.h"

#include <experimental/coroutine>

namespace concurrencpp::details {

	template<class type>
	class awaitable_base {

	protected:
		result_state_ptr<type> m_state;
		std::shared_ptr<::concurrencpp::executor> m_executor;
		bool m_force_rescheduling;

		void throw_if_empty(const char* message) const {
			if (static_cast<bool>(m_state)) {
				return;
			}

			throw concurrencpp::errors::empty_awaitable(message);
		}

		awaitable_base() noexcept = default;
		awaitable_base(awaitable_base&&) noexcept = default;
		
		awaitable_base(
			result_state_ptr<type> state_ptr,
			std::shared_ptr<concurrencpp::executor> executor,
			bool force_rescheduling) noexcept :
			m_state(std::move(state_ptr)),
			m_executor(std::move(executor)),
			m_force_rescheduling(force_rescheduling) {}

		void await_suspend(std::experimental::coroutine_handle<void> handle, const char* error_msg_if_empty) {
			throw_if_empty(error_msg_if_empty);
			m_state->await(std::move(m_executor), handle, m_force_rescheduling);
		}

	public:
		constexpr bool await_ready() const noexcept { return false; }
	};

}

namespace concurrencpp {
	template<class type>
	struct result_awaitable : public details::awaitable_base<type> {		
		result_awaitable() noexcept = default;
		result_awaitable(result_awaitable&&) noexcept = default;

		result_awaitable(
			details::result_state_ptr<type> state_ptr,
			std::shared_ptr<concurrencpp::executor> executor,
			bool force_rescheduling) noexcept :
			details::awaitable_base<type>(std::move(state_ptr), std::move(executor), force_rescheduling) {}

		void await_suspend(std::experimental::coroutine_handle<void> handle) {
			details::awaitable_base<type>::await_suspend(handle, "result_awaitable<type>::await_suspend - awaitable is empty.");
		}

		type await_resume() {
			auto state = std::move(this->m_state);
			return state->get();
		}
	};

	template<class type>
	struct result_resolver : public details::awaitable_base<type> {
		result_resolver() noexcept = default;
		result_resolver(result_resolver&&) noexcept = default;

		result_resolver(
			details::result_state_ptr<type> state_ptr,
			std::shared_ptr<concurrencpp::executor> executor,
			bool force_rescheduling) noexcept :
			details::awaitable_base<type>(std::move(state_ptr), std::move(executor), force_rescheduling) {}

		template<class handle_type>
		void await_suspend(handle_type handle) {
			details::awaitable_base<type>::await_suspend(handle, "result_resolver<type>::await_suspend - awaitable is empty.");
		}

		::concurrencpp::result<type> await_resume() noexcept {
			return ::concurrencpp::result<type>(std::move(this->m_state));
		}
	};
}

#endif