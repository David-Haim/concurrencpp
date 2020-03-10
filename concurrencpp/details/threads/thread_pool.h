#ifndef CONCURRENCPP_THREAD_POOL_H
#define CONCURRENCPP_THREAD_POOL_H

#include "../executors/task.h"
#include "../utils/spinlock.h"
#include "../utils/array_deque.h"
#include "thread_pool_listener.h"

#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>

#include <vector>
#include <stack>

#include <string_view>

namespace concurrencpp::details {
	class thread_pool;
	class thread_pool_worker;
	class waiting_worker_stack;

	class waiting_worker_stack {

	public:
		struct node {
			thread_pool_worker* const self;
			node* next;

			node(thread_pool_worker* worker) noexcept;
		};

	private:
		spinlock m_lock;
		node* m_head;
		
	public:
		waiting_worker_stack() noexcept;

		void push(node* worker_node) noexcept;
		thread_pool_worker* pop() noexcept;
	};

	class thread_pool_worker {

		enum class status {
			RUNNING,
			IDLE
		};

	private:
		std::mutex m_lock;
		array_deque<task> m_tasks;
		status m_status;
		std::condition_variable m_condition;
		std::thread m_thread;
		std::thread::id m_thread_id; //cached m_thread id.
		thread_pool& m_parent_pool;
		waiting_worker_stack::node m_node;

		static thread_local thread_pool_worker* tl_self_ptr;

		void assert_self_thread() const noexcept;
		void assert_idle_thread() const noexcept;

		void drain_local_queue();
		bool try_steal_task();
		
		bool wait_for_task();
		bool wait_for_task_impl(std::unique_lock<std::mutex>& lock);
		void exit(std::unique_lock<std::mutex>& lock);
		
		void dispose_worker(std::thread thread);
		void activate_worker(std::unique_lock<std::mutex>& lock);

		void work_loop() noexcept;
	
	public:
		thread_pool_worker(thread_pool& parent_pool) noexcept;
		thread_pool_worker(thread_pool_worker&&) noexcept;

		~thread_pool_worker() noexcept;

		void signal_termination();
		void join();

		std::thread::id enqueue_if_empty(task& task);
		std::thread::id enqueue(task& task, bool self);
		task try_donate_task() noexcept;

		void cancel_pending_tasks(std::exception_ptr reason) noexcept;

		static thread_pool_worker* this_thread_as_worker() noexcept;
		const thread_pool& get_parent_pool() const noexcept;
		waiting_worker_stack::node* get_waiting_node() noexcept;
	};

	class thread_pool {

	private:
		waiting_worker_stack m_waiting_workers;
		std::vector<thread_pool_worker> m_workers;
		std::atomic_size_t m_round_robin_index;
		const size_t m_max_workers;
		const std::string_view m_cancellation_msg;
		const std::shared_ptr<thread_pool_listener_base> m_listener;
		const std::chrono::seconds m_max_waiting_time;

	public:
		thread_pool(
			size_t max_worker_count,
			std::chrono::seconds max_waiting_time,
			std::string_view cancellation_msg,
			std::shared_ptr<thread_pool_listener_base> listener_ptr);

		~thread_pool() noexcept;

		std::thread::id enqueue(task task);

		//called by the workers
		bool try_steal_task(thread_pool_worker& worker);
		void mark_thread_waiting(thread_pool_worker& waiting_thread) noexcept;
		const std::shared_ptr<thread_pool_listener_base>& get_listener() const noexcept;
		std::chrono::seconds max_waiting_time() const noexcept;

		//mostly for testing:
		size_t index_of(thread_pool_worker* worker) const noexcept;
		bool is_current_thread_worker_thread() const noexcept;
	};
}

#endif //CONCURRENCPP_THREAD_POOL_H