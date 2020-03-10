#ifndef CONCURRENCPP_THREAD_EXECUTOR_H
#define CONCURRENCPP_THREAD_EXECUTOR_H

#include "executor.h"
#include "constants.h"

#include "../threads/thread.h"

#include <list>
#include <span>
#include <mutex>
#include <condition_variable>
#include <experimental/coroutine>

namespace concurrencpp::details {
	class thread_worker {

	private:
		thread m_thread;
		thread_executor& m_parent_pool;

		void execute_and_retire(
			std::experimental::coroutine_handle<> task,
			typename std::list<thread_worker>::iterator self_it);

	public:
		thread_worker(thread_executor& parent_pool) noexcept;
		~thread_worker() noexcept;

		void start(
			const std::string worker_name,
			std::experimental::coroutine_handle<> task,
			std::list<thread_worker>::iterator self_it);
	};
}

namespace concurrencpp {
	class alignas(64) thread_executor final : public executor {

		friend class ::concurrencpp::details::thread_worker;

	private:
		std::mutex m_lock;
		std::list<details::thread_worker> m_workers;
		std::condition_variable m_condition;
		std::list<details::thread_worker> m_last_retired;
		std::atomic_bool m_atomic_abort;

		void enqueue_impl(std::experimental::coroutine_handle<> task);

		void retire_worker(std::list<details::thread_worker>::iterator it);

	public:
		thread_executor() :
			executor(details::consts::k_thread_executor_name),
			m_atomic_abort(false) {}

		~thread_executor() noexcept;

		void enqueue(std::experimental::coroutine_handle<> task) override;
		void enqueue(std::span<std::experimental::coroutine_handle<>> tasks) override;

		int max_concurrency_level() const noexcept override;

		bool shutdown_requested() const noexcept override;
		void shutdown() noexcept override;
	};
}

#endif