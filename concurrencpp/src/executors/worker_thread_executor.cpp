#include "worker_thread_executor.h"
#include "constants.h"

#include "../errors.h"

namespace concurrencpp::details {
	static thread_local worker_thread_executor* s_tl_this_worker = nullptr;
}

using concurrencpp::worker_thread_executor;

worker_thread_executor::worker_thread_executor() :
	executor(details::consts::k_worker_thread_executor_name),
	m_private_atomic_abort(false),
	m_abort(false),
	m_atomic_abort(false) {
	m_thread = details::thread(details::make_executor_worker_name(name), [this] { work_loop(); });
}

worker_thread_executor::~worker_thread_executor() noexcept {
	assert(!m_thread.joinable());
}

void worker_thread_executor::join() {
	{
		std::unique_lock<decltype(m_lock)> lock(m_lock);
		m_abort = true;
	}

	m_condition.notify_one();
	m_thread.join();
}

void worker_thread_executor::destroy_tasks() noexcept {
	for (auto task : m_private_queue) {
		task.destroy();
	}

	m_private_queue.clear();

	{
		std::unique_lock<std::mutex> lock(m_lock);
		m_private_queue = std::move(m_public_queue);
	}

	for (auto task : m_private_queue) {
		task.destroy();
	}
}

bool worker_thread_executor::drain_queue_impl() {
	while (!m_private_queue.empty()) {
		auto task = m_private_queue.front();
		m_private_queue.pop_front();

		if (m_private_atomic_abort.load(std::memory_order_relaxed)) {
			return false;
		}

		task();
	}

	return true;
}

bool worker_thread_executor::drain_queue() {
	assert(m_private_queue.empty());
	m_private_queue = {};

	std::unique_lock<decltype(m_lock)> lock(m_lock);
	m_condition.wait(lock, [this] {
		return !m_public_queue.empty() || m_abort;
	});

	if (m_abort) {
		return false;
	}

	m_private_queue = std::move(m_public_queue);
	lock.unlock();

	return drain_queue_impl();
}

void worker_thread_executor::work_loop() noexcept {
	details::s_tl_this_worker = this;

	while (true) {
		if (!drain_queue()) {
			return;
		}
	}
}

void worker_thread_executor::enqueue_local(std::experimental::coroutine_handle<> task) {
	if (!m_private_atomic_abort.load(std::memory_order_relaxed)) {
		m_private_queue.emplace_back(task);
		return;
	}

	details::throw_executor_shutdown_exception(name);
}

void worker_thread_executor::enqueue_local(std::span<std::experimental::coroutine_handle<>> tasks) {
	if (!m_private_atomic_abort.load(std::memory_order_relaxed)) {
		m_private_queue.insert(m_private_queue.end(), tasks.begin(), tasks.end());
		return;
	}

	details::throw_executor_shutdown_exception(name);
}

void worker_thread_executor::enqueue_foreign(std::experimental::coroutine_handle<> task) {
	{
		std::unique_lock<decltype(m_lock)> lock(m_lock);
		if (m_abort) {
			details::throw_executor_shutdown_exception(name);
		}

		m_public_queue.emplace_back(task);
	}

	m_condition.notify_one();
}

void worker_thread_executor::enqueue_foreign(std::span<std::experimental::coroutine_handle<>> tasks) {
	{
		std::unique_lock<decltype(m_lock)> lock(m_lock);
		if (m_abort) {
			details::throw_executor_shutdown_exception(name);
		}

		m_public_queue.insert(m_public_queue.end(), tasks.begin(), tasks.end());
	}

	m_condition.notify_one();
}

void worker_thread_executor::enqueue(std::experimental::coroutine_handle<> task) {
	if (details::s_tl_this_worker == this) {
		return enqueue_local(task);
	}

	enqueue_foreign(task);
}

void worker_thread_executor::enqueue(std::span<std::experimental::coroutine_handle<>> tasks) {
	if (details::s_tl_this_worker == this) {
		return enqueue_local(tasks);
	}

	enqueue_foreign(tasks);
}

int worker_thread_executor::max_concurrency_level() const noexcept {
	return 1;
}

bool concurrencpp::worker_thread_executor::shutdown_requested() const noexcept {
	return m_atomic_abort.load(std::memory_order_relaxed);
}

void worker_thread_executor::shutdown() noexcept {
	m_private_atomic_abort.store(true, std::memory_order_relaxed);
	m_atomic_abort.store(true, std::memory_order_relaxed);

	join();
	destroy_tasks();
}