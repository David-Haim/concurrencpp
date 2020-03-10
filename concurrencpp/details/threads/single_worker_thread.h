#ifndef CONCURRENCPP_SINGLE_WORKER_THREAD_H
#define CONCURRENCPP_SINGLE_WORKER_THREAD_H

#include "../executors/task.h"
#include "../utils/array_deque.h"

#include <string>
#include <mutex>
#include <thread>
#include <condition_variable>

namespace concurrencpp::details {
	class single_worker_thread {

	private:
		std::mutex m_lock;
		std::condition_variable m_condition;
		array_deque<task> m_tasks;
		std::thread m_thread;

		void work_loop() noexcept;

	public:
		single_worker_thread();
		~single_worker_thread() noexcept;

		void enqueue(task task);
	};
}

#endif