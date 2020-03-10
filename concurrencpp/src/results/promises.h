#ifndef CONCURRENCPP_COROUTINE_STATE_H
#define CONCURRENCPP_COROUTINE_STATE_H

#include "result_core.h"

#include "../errors.h"

namespace concurrencpp::details {
	struct initial_awaiter : public std::experimental::suspend_always {
		bool await_suspend(std::experimental::coroutine_handle<> handle) const {
			return result_core_base::initial_reschedule(handle);
		}
	};

	struct coro_promise_base {
		template<class ... argument_types>
		static void* operator new (size_t size, argument_types&& ...) {
			return ::operator new(size);
		}

		template<class executor_type, class ... argument_types>
		static void* operator new (
			size_t size,
			executor_tag,
			executor_type* executor,
			argument_types&& ...) {

			static_assert(
				std::is_base_of_v<concurrencpp::executor, executor_type>,
				"concurrencpp::<<coroutine>> - first argument is executor_tag but second argument is not driven from concurrencpp::executor.");

			assert(executor);
			assert(result_core_per_thread_data::s_tl_per_thread_data.executor == nullptr);
			result_core_per_thread_data::s_tl_per_thread_data.executor = executor;

			return ::operator new(size);
		}

		template<class executor_type, class ... argument_types>
		static void* operator new (
			size_t size,
			executor_tag,
			std::shared_ptr<executor_type> executor,
			argument_types&& ... args) {

		    return operator new(size, executor_tag{}, executor.get(), std::forward<argument_types>(args)...);
		}

		template<class ... argument_types>
		static void* operator new (
			size_t size,
			executor_bulk_tag,
			std::vector<std::experimental::coroutine_handle<>>* accumulator,
			argument_types&& ...) {

			assert(accumulator != nullptr);
			assert(result_core_per_thread_data::s_tl_per_thread_data.accumulator == nullptr);
			result_core_per_thread_data::s_tl_per_thread_data.accumulator = accumulator;

			return ::operator new(size);
		}
	};

	struct task_promise : public coro_promise_base {
		null_result get_return_object() const noexcept {
			return {};
		}

		initial_awaiter initial_suspend() const noexcept {
			return {};
		}

		std::experimental::suspend_never final_suspend() const noexcept {
			return {};
		}

		void unhandled_exception() const noexcept {}
		void return_void() const noexcept {}
	};
}

namespace concurrencpp::details {
	template<class derived_type, class type>
	struct return_value_struct {
		template<class return_type>
		void return_value(return_type&& value) {
			auto self = static_cast<derived_type*>(this);
			self->set_result(std::forward<return_type>(value));
		}
	};

	template<class derived_type>
	struct return_value_struct<derived_type, void> {
		void return_void() noexcept {
			auto self = static_cast<derived_type*>(this);
			self->set_result();
		}
	};

	template<class type>
	struct result_publisher : public std::experimental::suspend_always {
		bool await_suspend(coro_handle<type> handle) const noexcept {
		 	handle.promise().publish_result();
			return false; //don't suspend, resume and destroy this
		}
	};

	template<class type>
	class coroutine_state :
		public coro_promise_base,
		public return_value_struct<coroutine_state<type>, type> {

	private:
		std::shared_ptr<result_core<type>> m_result_ptr;

	public:
		coroutine_state() : m_result_ptr(std::make_shared<result_core<type>>()) {}

		~coroutine_state() noexcept {
			if (!static_cast<bool>(this->m_result_ptr)) {
				return;
			}

			auto broken_task_error =
				std::make_exception_ptr(concurrencpp::errors::broken_task("coroutine was destroyed before finishing execution"));
			this->m_result_ptr->set_exception(broken_task_error);
			this->m_result_ptr->publish_result();
		}

		template<class ... argument_types>
		void set_result(argument_types&& ... args) {
			this->m_result_ptr->set_result(std::forward<argument_types>(args)...);
		}

		void unhandled_exception() noexcept {
			this->m_result_ptr->set_exception(std::current_exception());
		}

		::concurrencpp::result<type> get_return_object() noexcept {
			return { this->m_result_ptr };
		}

		void publish_result() noexcept {
			this->m_result_ptr->publish_result();
			this->m_result_ptr.reset();
		}

		initial_awaiter initial_suspend() const noexcept {
			return {};
		}

		result_publisher<type> final_suspend() const noexcept {
			return {};
		}
	};
}

namespace std::experimental {
	template<class... arguments>
	struct coroutine_traits<::concurrencpp::null_result, arguments...> {
		using promise_type = concurrencpp::details::task_promise;
	};

	template<class type, class... arguments>
	struct coroutine_traits<::concurrencpp::result<type>, arguments...> {
		using promise_type = concurrencpp::details::coroutine_state<type>;
	};
}

#endif