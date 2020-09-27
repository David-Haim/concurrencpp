#ifndef CONCURRENCPP_RUNTIME_H
#define CONCURRENCPP_RUNTIME_H

#include "constants.h"

#include "../forward_declerations.h"

#include <memory>
#include <mutex>
#include <vector>
#include <chrono>

namespace concurrencpp::details {
	class executor_collection {

	private:
		std::mutex m_lock;
		std::vector<std::shared_ptr<executor>> m_executors;

	public:
		void register_executor(std::shared_ptr<executor> executor);
		void shutdown_all() noexcept;
	};
}

namespace concurrencpp {
	struct runtime_options {
		size_t max_cpu_threads;
		std::chrono::seconds max_cpu_thread_waiting_time;

		size_t max_background_threads;
		std::chrono::seconds max_background_thread_waiting_time;

		runtime_options() noexcept;

		runtime_options(const runtime_options&) noexcept = default;
		runtime_options& operator = (const runtime_options&) noexcept = default;
	};

	class runtime {

	private:
		std::shared_ptr<timer_queue> m_timer_queue;

		std::shared_ptr<inline_executor> m_inline_executor;
		std::shared_ptr<thread_pool_executor> m_thread_pool_executor;
		std::shared_ptr<thread_pool_executor> m_background_executor;
		std::shared_ptr<thread_executor> m_thread_executor;

		details::executor_collection m_registered_executors;

	public:
		runtime();
		runtime(const concurrencpp::runtime_options& options);

		~runtime() noexcept;

		std::shared_ptr<concurrencpp::timer_queue> timer_queue() const noexcept;

		std::shared_ptr<concurrencpp::inline_executor> inline_executor() const noexcept;
		std::shared_ptr<concurrencpp::thread_pool_executor> thread_pool_executor() const noexcept;
		std::shared_ptr<concurrencpp::thread_pool_executor> background_executor() const noexcept;
		std::shared_ptr<concurrencpp::thread_executor> thread_executor() const noexcept;

		std::shared_ptr<concurrencpp::worker_thread_executor> make_worker_thread_executor();
		std::shared_ptr<concurrencpp::manual_executor> make_manual_executor();

		static std::tuple<unsigned int, unsigned int, unsigned int> version() noexcept;

		template<class executor_type, class ... argument_types>
		std::shared_ptr<executor_type> make_executor(argument_types&& ... arguments) {
			static_assert(std::is_base_of_v<concurrencpp::executor, executor_type>,
				"concurrencpp::runtime::make_executor - <<executor_type>> is not a derived class of concurrencpp::executor.");

			static_assert(std::is_constructible_v<executor_type, argument_types...>,
				"concurrencpp::runtime::make_executor - can not build <<executor_type>> from <<argument_types...>>.");

			static_assert(!std::is_abstract_v<executor_type>,
				"concurrencpp::runtime::make_executor - <<executor_type>> is an abstract class.");

			auto executor = std::make_shared<executor_type>(std::forward<argument_types>(arguments)...);
			m_registered_executors.register_executor(executor);
			return executor;
		}
	};
}

#endif