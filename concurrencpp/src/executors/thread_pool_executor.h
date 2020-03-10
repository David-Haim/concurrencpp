#ifndef CONCURRENCPP_THREAD_POOL_EXECUTOR_H
#define CONCURRENCPP_THREAD_POOL_EXECUTOR_H

#include "executor.h"

#include <mutex>
#include <atomic>
#include <vector>
#include <deque>
#include <condition_variable>
#include <experimental/coroutine>

#include "../threads/thread.h"

namespace concurrencpp::details {
	class idle_worker_set;
	class thread_pool_worker;
	class thread_pool;
}

namespace concurrencpp::details {
	class idle_worker_set {

		enum class status {
			active,
			idle
		};

		struct alignas(64) padded_flag {
			std::atomic<status> flag;
			const char padding[64 - sizeof(flag)] = {};
		};

	private:
		std::atomic_intptr_t m_approx_size;
		const std::unique_ptr<padded_flag[]> m_idle_flags;
		const size_t m_size;

		bool try_acquire_flag(size_t index) noexcept;

	public:
		idle_worker_set(size_t size);

		void set_idle(size_t idle_thread) noexcept;
		void set_active(size_t idle_thread) noexcept;

		size_t find_idle_worker(size_t caller_index) noexcept;
		void find_idle_workers(size_t caller_index, std::vector<size_t>& result_buffer, size_t max_count) noexcept;
	};
}

namespace concurrencpp::details {
	class alignas(64) thread_pool_worker {

		enum class status {
			working,
			waiting,
			idle
		};

	private:
		std::deque<std::experimental::coroutine_handle<>> m_private_queue;
		std::vector<size_t> m_idle_worker_list;
		std::atomic_bool m_atomic_abort;
		thread_pool_executor& m_parent_pool;
		const size_t m_index;
		const size_t m_pool_size;
		const std::chrono::seconds m_max_idle_time;
		const std::string m_worker_name;
		const char m_padding[64] = {};
		std::mutex m_lock;
		status m_status;
		std::deque<std::experimental::coroutine_handle<>> m_public_queue;
		thread m_thread;
		std::condition_variable m_condition;
		bool m_abort;

		void balance_work();

		bool wait_for_task(std::unique_lock<std::mutex>& lock) noexcept;
		bool drain_queue_impl();
		bool drain_queue();

		void work_loop() noexcept;

		void destroy_tasks() noexcept;
		void ensure_worker_active(std::unique_lock<std::mutex>& lock);

	public:
		thread_pool_worker(
			thread_pool_executor& parent_pool,
			size_t index,
			size_t pool_size,
			std::chrono::seconds max_idle_time);

		thread_pool_worker(thread_pool_worker&& rhs) noexcept;
		~thread_pool_worker() noexcept;

		void enqueue_foreign(std::experimental::coroutine_handle<> task);
		void enqueue_foreign(std::span<std::experimental::coroutine_handle<>> tasks);

		void enqueue_local(std::experimental::coroutine_handle<> task);
		void enqueue_local(std::span<std::experimental::coroutine_handle<>> tasks);

		void abort() noexcept;
		void join() noexcept;
	};
}

namespace concurrencpp {
	class alignas (64) thread_pool_executor final : public executor{

		friend class details::thread_pool_worker;

	private:
		std::vector<details::thread_pool_worker> m_workers;
		const char m_padding_0[64 - sizeof(m_workers)] = {};
		std::atomic_size_t m_round_robin_cursor;
		const char m_padding_1[64 - sizeof(m_round_robin_cursor)] = {};
		details::idle_worker_set m_idle_workers;
		const char m_padding_2[64 - sizeof(m_idle_workers)] = {};
		std::atomic_bool m_abort;

		void mark_worker_idle(size_t index) noexcept;
		void mark_worker_active(size_t index) noexcept;

		void find_idle_workers(size_t caller_index, std::vector<size_t>& buffer, size_t max_count) noexcept;

		details::thread_pool_worker& worker_at(size_t index) noexcept {
			return m_workers[index];
		}

	public:
		thread_pool_executor(std::string_view name, size_t size, std::chrono::seconds max_idle_time);

		~thread_pool_executor() noexcept;

		void enqueue(std::experimental::coroutine_handle<> task) override;
		void enqueue(std::span<std::experimental::coroutine_handle<>> tasks) override;

		int max_concurrency_level() const noexcept override;

		bool shutdown_requested() const noexcept override;
		void shutdown() noexcept override;
	};
}

#endif