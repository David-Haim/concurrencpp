#ifndef CONCURRENCPP_MANUAL_EXECUTOR_H
#define CONCURRENCPP_MANUAL_EXECUTOR_H

#include "executor.h"
#include "constants.h"

#include <deque>

namespace concurrencpp {
	class alignas(64) manual_executor final : public executor {

	private:
		mutable std::mutex m_lock;
		std::deque<std::experimental::coroutine_handle<>> m_tasks;
		std::condition_variable m_condition;
		bool m_abort;
		std::atomic_bool m_atomic_abort;

		void destroy_tasks() noexcept;

	public:
		manual_executor() :
			executor(details::consts::k_manual_executor_name),
			m_abort(false),
			m_atomic_abort(false) {};

		void enqueue(std::experimental::coroutine_handle<> task) override;
		void enqueue(std::span<std::experimental::coroutine_handle<>> tasks) override;

		int max_concurrency_level() const noexcept override;

		void shutdown() noexcept override;
		bool shutdown_requested() const noexcept override;

		size_t size() const noexcept;
		bool empty() const noexcept;

		bool loop_once();
		bool loop_once(std::chrono::milliseconds max_waiting_time);

		size_t loop(size_t max_count);

		size_t clear() noexcept;

		void wait_for_task();
		bool wait_for_task(std::chrono::milliseconds max_waiting_time);
	};
}

#endif