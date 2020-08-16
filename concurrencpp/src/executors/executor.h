#ifndef CONCURRENCPP_EXECUTOR_H
#define CONCURRENCPP_EXECUTOR_H

#include "../results/result.h"

#include <span>
#include <string>
#include <string_view>

namespace concurrencpp::details {
	[[noreturn]] void throw_executor_shutdown_exception(std::string_view executor_name);
	std::string make_executor_worker_name(std::string_view executor_name);
}

namespace concurrencpp {
	class executor {

	private:
		template<class callable_type, class ... argument_types>
		static null_result post_bridge(
			executor_tag,
			executor*,
			callable_type callable,
			argument_types... arguments) {
			callable(arguments...);
			co_return;
		}

		template<class callable_type>
		static null_result bulk_post_bridge(
			details::executor_bulk_tag,
			std::vector<std::experimental::coroutine_handle<>>* accumulator,
			callable_type callable) {
			callable();
			co_return;
		}

		template<class return_type, class callable_type, class ... argument_types>
		static result<return_type> submit_bridge(
				executor_tag,
				executor*,
				callable_type callable,
				argument_types... arguments) {
			co_return callable(arguments...);
		}

		template<class callable_type, class return_type = typename std::invoke_result_t<callable_type>>
			static result<return_type> bulk_submit_bridge(
				details::executor_bulk_tag,
				std::vector<std::experimental::coroutine_handle<>>* accumulator,
				callable_type callable) {
			co_return callable();
		}

	public:
		executor(std::string_view name) : name(name) {}

		virtual ~executor() noexcept = default;

		const std::string name;

		virtual void enqueue(std::experimental::coroutine_handle<> task) = 0;
		virtual void enqueue(std::span<std::experimental::coroutine_handle<>> tasks) = 0;

		virtual int max_concurrency_level() const noexcept = 0;

		virtual bool shutdown_requested() const noexcept = 0;
		virtual void shutdown() noexcept = 0;

		template<class callable_type, class ... argument_types>
		void post(callable_type&& callable, argument_types&& ... arguments) {
			static_assert(
				std::is_invocable_v<callable_type, argument_types...>,
				"concurrencpp::executor::post - <<callable_type>> is not invocable with <<argument_types...>>");

			post_bridge(
				executor_tag{},
				this,
				std::forward<callable_type>(callable),
				std::forward<argument_types>(arguments)...);
		}

		template<class callable_type, class ... argument_types>
		auto submit(callable_type&& callable, argument_types&& ... arguments) {
			static_assert(
				std::is_invocable_v<callable_type, argument_types...>,
				"concurrencpp::executor::submit - <<callable_type>> is not invocable with <<argument_types...>>");

			using return_type = typename std::invoke_result_t<callable_type, argument_types...>;

			return submit_bridge<return_type>(
				executor_tag{},
				this,
				std::forward<callable_type>(callable),
				std::forward<argument_types>(arguments)...);
		}

		template<class callable_type>
		void bulk_post(std::span<callable_type> callable_list) {
			std::vector<std::experimental::coroutine_handle<>> accumulator;
			accumulator.reserve(callable_list.size());

			for (auto& callable : callable_list) {
				bulk_post_bridge<callable_type>({}, &accumulator, std::move(callable));
			}

			assert(!accumulator.empty());
			enqueue(accumulator);
		}

		template<class callable_type, class return_type = std::invoke_result_t<callable_type>>
		std::vector<concurrencpp::result<return_type>> bulk_submit(std::span<callable_type> callable_list) {
			std::vector<std::experimental::coroutine_handle<>> accumulator;
			accumulator.reserve(callable_list.size());

			std::vector<concurrencpp::result<return_type>> results;
			results.reserve(callable_list.size());

			for (auto& callable : callable_list) {
				results.emplace_back(bulk_submit_bridge<callable_type>({}, &accumulator, std::move(callable)));
			}

			assert(!accumulator.empty());
			enqueue(accumulator);
			return results;
		}
	};
}

#endif