#include "executors.h"
#include "constants.h"
#include "../errors.h"
#include "../threads/thread_pool.h"
#include "../threads/thread_group.h"
#include "../runtime/runtime.h"

#include <numeric>

using concurrencpp::task;
using concurrencpp::inline_executor;
using concurrencpp::thread_pool_executor;
using concurrencpp::parallel_executor;
using concurrencpp::background_executor;
using concurrencpp::thread_executor;
using concurrencpp::worker_thread_executor;
using concurrencpp::manual_executor;

using concurrencpp::details::thread_group;
using concurrencpp::details::thread_pool;

/*
	inline_executor
*/

inline_executor::inline_executor(const inline_executor::context&) noexcept {}

std::string_view inline_executor::name() const noexcept {
	return details::consts::k_inline_executor_name;
}

void inline_executor::enqueue(task task) {
	task();
}

/*
	thread_pool_executor
*/

thread_pool_executor::thread_pool_executor(const thread_pool_executor::context& ctx) : 
	m_cpu_thread_pool(ctx.max_worker_count, ctx.max_waiting_time, ctx.cancellation_msg, nullptr){}

std::string_view thread_pool_executor::name() const noexcept {
	return details::consts::k_thread_pool_executor_name;
}

void thread_pool_executor::enqueue(task task) {
	m_cpu_thread_pool.enqueue(std::move(task));
}

/*
	background_executor
*/

background_executor::background_executor(const background_executor::context& ctx) :
	m_background_thread_pool(ctx.max_worker_count, ctx.max_waiting_time, ctx.cancellation_msg, nullptr) {}

std::string_view background_executor::name() const noexcept {
	return details::consts::k_background_executor_name;
}

void background_executor::enqueue(task task) {
	m_background_thread_pool.enqueue(std::move(task));
}

/*
	thread_executor
*/

thread_executor::thread_executor(const thread_executor::context&) noexcept : 
	m_thread_group(nullptr){}

std::string_view thread_executor::name() const noexcept {
	return details::consts::k_thread_executor_name;
}

void thread_executor::enqueue(task task) {
	m_thread_group.enqueue(std::move(task));
}

/*
	worker_thread_executor
*/

worker_thread_executor::worker_thread_executor(const worker_thread_executor::context&){}

std::string_view worker_thread_executor::name() const noexcept {
	return details::consts::k_worker_thread_executor_name;
}

void worker_thread_executor::enqueue(task task) {
	m_worker.enqueue(std::move(task));
}

/*
	manual_executor
*/

manual_executor::manual_executor(const manual_executor::context&){}

manual_executor::~manual_executor() noexcept {
	concurrencpp::errors::broken_task error(details::consts::k_manual_executor_cancel_error_msg);
	const auto exception_ptr = std::make_exception_ptr(error);

	std::unique_lock<decltype(m_lock)> lock(m_lock);
	for (auto& task : m_tasks) {
		task.cancel(exception_ptr);
	}
}

std::string_view manual_executor::name() const noexcept {
	return details::consts::k_manual_executor_name;
}

void manual_executor::enqueue(task task) {
	std::unique_lock<decltype(m_lock)> lock(m_lock);
	m_tasks.emplace_back(std::move(task));

	lock.unlock();
	m_condition.notify_all();
}

size_t manual_executor::size() const noexcept {
	std::unique_lock<decltype(m_lock)> lock(m_lock);
	return m_tasks.size();
}

bool manual_executor::empty() const noexcept {
	return size() == 0;
}

bool manual_executor::loop_once() {
	task task;
	
	std::unique_lock<decltype(m_lock)> lock(m_lock);
	if (m_tasks.empty()) {
		return false;
	}

	task = m_tasks.pop_front();
	lock.unlock();

	task();
	return true;
}

size_t manual_executor::loop(size_t counts) {
	size_t executed = 0;
	for ( ; executed < counts ;  ++executed) {
		if (!loop_once()) {
			break;
		}
	}

	return executed;
}

void manual_executor::cancel_all(std::exception_ptr reason) {
	std::unique_lock<decltype(m_lock)> lock(m_lock);
	for (auto& task : m_tasks) {
		task.cancel(reason);
	}

	m_tasks.clear();
}

void manual_executor::wait_for_task() {
	std::unique_lock<decltype(m_lock)> lock(m_lock);
	if (!m_tasks.empty()) {
		return;
	}

	m_condition.wait(lock, [this] { return !m_tasks.empty(); });
	assert(!m_tasks.empty());
}

bool manual_executor::wait_for_task(std::chrono::milliseconds max_waiting_time) {
	std::unique_lock<decltype(m_lock)> lock(m_lock);
	if (!m_tasks.empty()) {
		return false;
	}

	return m_condition.wait_for(lock, max_waiting_time, [this] { return !m_tasks.empty(); });
}