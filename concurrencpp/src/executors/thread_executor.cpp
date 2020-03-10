#include "thread_executor.h"
#include "constants.h"

using concurrencpp::thread_executor;
using concurrencpp::details::thread_worker;

thread_worker::thread_worker(thread_executor& parent_pool) noexcept :
	m_parent_pool(parent_pool) {}

thread_worker::~thread_worker() noexcept {
	m_thread.join();
}

void thread_worker::execute_and_retire(
	std::experimental::coroutine_handle<> task,
	typename std::list<thread_worker>::iterator self_it) {
	task();
	m_parent_pool.retire_worker(self_it);
}

void thread_worker::start(
	std::string worker_name,
	std::experimental::coroutine_handle<> task,
	std::list<thread_worker>::iterator self_it) {
	m_thread = thread(std::move(worker_name), [this, task, self_it] {
		execute_and_retire(task, self_it);
	});
}

thread_executor::~thread_executor() noexcept {
	assert(m_workers.empty());
	assert(m_last_retired.empty());
}

void thread_executor::enqueue_impl(std::experimental::coroutine_handle<> task) {
	m_workers.emplace_front(*this);
	m_workers.front().start(details::make_executor_worker_name(name), task, m_workers.begin());
}

void thread_executor::enqueue(std::experimental::coroutine_handle<> task) {
	if (m_atomic_abort.load(std::memory_order_relaxed)) {
		details::throw_executor_shutdown_exception(name);
	}

	std::unique_lock<decltype(m_lock)> lock(m_lock);
	enqueue_impl(task);
}

void thread_executor::enqueue(std::span<std::experimental::coroutine_handle<>> tasks) {
	if (m_atomic_abort.load(std::memory_order_relaxed)) {
		details::throw_executor_shutdown_exception(name);
	}

	std::unique_lock<decltype(m_lock)> lock(m_lock);
	for (auto task : tasks) {
		enqueue_impl(task);
	}
}

int thread_executor::max_concurrency_level() const noexcept {
	return details::consts::k_thread_executor_max_concurrency_level;
}

bool thread_executor::shutdown_requested() const noexcept {
	return m_atomic_abort.load(std::memory_order_relaxed);
}

void thread_executor::shutdown() noexcept {
	m_atomic_abort.store(true, std::memory_order_relaxed);

	std::unique_lock<decltype(m_lock)> lock(m_lock);
	m_condition.wait(lock, [this] { return m_workers.empty(); });
	m_last_retired.clear();
}

void thread_executor::retire_worker(std::list<thread_worker>::iterator it) {
	std::unique_lock<decltype(m_lock)> lock(m_lock);
	auto last_retired = std::move(m_last_retired);
	m_last_retired.splice(m_last_retired.begin(), m_workers, it);

	lock.unlock();
	m_condition.notify_one();

	last_retired.clear();
}