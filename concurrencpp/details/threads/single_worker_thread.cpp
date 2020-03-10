#include "single_worker_thread.h"

#include "../errors.h"
#include "../runtime/runtime.h"
#include "../executors/constants.h"

using concurrencpp::details::thread_interrupter;
using concurrencpp::details::single_worker_thread;
using concurrencpp::task;

single_worker_thread::single_worker_thread() :
	m_thread([this] { work_loop(); }) {}

single_worker_thread::~single_worker_thread() noexcept {	
	concurrencpp::errors::broken_task error(details::consts::k_worker_thread_executor_cancel_error_msg);
	const auto exception_ptr = std::make_exception_ptr(error);

	{
		std::unique_lock<decltype(m_lock)> lock(m_lock);
		m_tasks.emplace_front(interrupt_thread);
	}

	m_condition.notify_one();

	assert(m_thread.joinable());
	m_thread.join();

	std::unique_lock<decltype(m_lock)> lock(m_lock);
	for (auto& task : m_tasks) {
		task.cancel(exception_ptr);
	}
}

void single_worker_thread::work_loop() noexcept {
	try {
		while (true) {	
			task task;
			
			std::unique_lock<decltype(m_lock)> lock(m_lock);
			if (m_tasks.empty()) {
				m_condition.wait(lock, [this] { return !m_tasks.empty(); });
			}

			assert(!m_tasks.empty());
			task = m_tasks.pop_front();
			lock.unlock();

			assert(static_cast<bool>(task));
			task();
		}
	}
	catch (const thread_interrupter&) {
		return;
	}
	catch (...) {
		std::abort();
	}
}

void single_worker_thread::enqueue(task task) {
	{
		std::unique_lock<decltype(m_lock)> lock(m_lock);
		m_tasks.emplace_back(std::move(task));
	}

	m_condition.notify_one();
}