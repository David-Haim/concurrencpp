#ifndef CONCURRENCPP_RESULT_AWAITABLE_H
#define CONCURRENCPP_RESULT_AWAITABLE_H

#include "constants.h"
#include "result_fwd_declerations.h"

#include "../errors.h"

namespace concurrencpp {
	template<class type>
	class awaitable : public std::experimental::suspend_always {

	private:
		std::shared_ptr<details::result_core<type>> m_state;

	public:
		awaitable(std::shared_ptr<details::result_core<type>> state) noexcept :
			m_state(std::move(state)) {}

		awaitable(awaitable&& rhs) noexcept = default;

		bool await_suspend(std::experimental::coroutine_handle<> caller_handle) {
			if (!static_cast<bool>(m_state)) {
				throw concurrencpp::errors::empty_awaitable(details::consts::k_result_awaitable_error_msg);
			}

			return m_state->await(caller_handle);
		}

		type await_resume() {
			auto state = std::move(m_state);
			return state->get();
		}
	};

	template<class type>
	class via_awaitable : public std::experimental::suspend_always {

	private:
		std::shared_ptr<details::result_core<type>> m_state;
		std::shared_ptr<executor> m_executor;
		const bool m_force_rescheduling;

	public:
		via_awaitable(
			std::shared_ptr<details::result_core<type>> state,
			std::shared_ptr<concurrencpp::executor> executor,
			bool force_rescheduling) noexcept :
			m_state(std::move(state)),
			m_executor(std::move(executor)),
			m_force_rescheduling(force_rescheduling) {}

		via_awaitable(via_awaitable&& rhs) noexcept = default;

		bool await_suspend(std::experimental::coroutine_handle<> caller_handle) {
			if (!static_cast<bool>(m_state)) {
				throw concurrencpp::errors::empty_awaitable(details::consts::k_result_awaitable_error_msg);
			}

			return m_state->await_via(std::move(m_executor), caller_handle, m_force_rescheduling);
		}

		type await_resume() {
			auto state = std::move(m_state);
			return state->get();
		}
	};

	template<class type>
	class resolve_awaitable final : public std::experimental::suspend_always {

	private:
		std::shared_ptr<details::result_core<type>> m_state;

	public:
		resolve_awaitable(std::shared_ptr<details::result_core<type>> state) noexcept :
			m_state(std::move(state)) {}

		resolve_awaitable(resolve_awaitable&&) noexcept = default;

		bool await_suspend(std::experimental::coroutine_handle<> caller_handle) {
			if (!static_cast<bool>(m_state)) {
				throw concurrencpp::errors::empty_awaitable(details::consts::k_result_awaitable_error_msg);
			}

			return m_state->await(caller_handle);
		}

		result<type> await_resume() {
			return result<type>(std::move(m_state));
		}
	};

	template<class type>
	class resolve_via_awaitable final : public std::experimental::suspend_always {

	private:
		std::shared_ptr<details::result_core<type>> m_state;
		std::shared_ptr<concurrencpp::executor> m_executor;
		const bool m_force_rescheduling;

	public:
		resolve_via_awaitable(
			std::shared_ptr<details::result_core<type>> state,
			std::shared_ptr<concurrencpp::executor> executor,
			bool force_rescheduling) noexcept :
			m_state(state),
			m_executor(std::move(executor)),
			m_force_rescheduling(force_rescheduling) {}

		resolve_via_awaitable(resolve_via_awaitable&&) noexcept = default;

		bool await_suspend(std::experimental::coroutine_handle<> caller_handle) {
			if (!static_cast<bool>(m_state)) {
				throw concurrencpp::errors::empty_awaitable(details::consts::k_result_awaitable_error_msg);
			}

			return m_state->await_via(std::move(m_executor), caller_handle, m_force_rescheduling);
		}

		result<type> await_resume() noexcept {
			return result<type>(std::move(m_state));
		}
	};
}

#endif