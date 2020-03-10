#include "thread_pool_executor.h"
#include "constants.h"

#include <array>

#include <ctime>
#include <cmath>

using concurrencpp::thread_pool_executor;
using concurrencpp::details::idle_worker_set;
using concurrencpp::details::thread_pool_worker;

namespace concurrencpp::details {
	class randomizer {

	private:
		uint32_t m_seed;

	public:
		randomizer(uint32_t seed) noexcept : m_seed(seed) {}

		uint32_t operator()() noexcept {
			m_seed = (214013 * m_seed + 2531011);
			return (m_seed >> 16) & 0x7FFF;
		}
	};

	struct thread_pool_per_thread_data {
		thread_pool_worker* this_worker;
		randomizer randomizer;
		size_t this_thread_index;

		thread_pool_per_thread_data() noexcept :
			this_worker(nullptr),
			randomizer(static_cast<uint32_t>(std::rand())),
			this_thread_index(static_cast<size_t>(-1)){}
	};

	static thread_local thread_pool_per_thread_data s_tl_thread_pool_data;
}

idle_worker_set::idle_worker_set(size_t size) :
	m_approx_size(0),
	m_idle_flags(std::make_unique<padded_flag[]>(size)),
	m_size(size) {
	for (size_t i = 0; i < size; i++) {
		m_idle_flags[i].flag = status::active;
	}
}

void idle_worker_set::set_idle(size_t idle_thread) noexcept {
	m_idle_flags[idle_thread].flag.store(status::idle, std::memory_order_relaxed);
	m_approx_size.fetch_add(1, std::memory_order_release); //the mo is in order for the addition to happen AFTER flagging.
}

void idle_worker_set::set_active(size_t idle_thread) noexcept {
	auto expected = status::idle;
	const auto swapped = m_idle_flags[idle_thread].flag.compare_exchange_strong(
		expected,
		status::active,
		std::memory_order_relaxed);

	if (!swapped) {
		return;
	}

	m_approx_size.fetch_sub(1, std::memory_order_release);
}

bool idle_worker_set::try_acquire_flag(size_t index) noexcept {
	const auto worker_status = m_idle_flags[index].flag.load(std::memory_order_relaxed);
	if (worker_status == status::active) {
		return false;
	}

	auto expected = status::idle;
	const auto swapped = m_idle_flags[index].flag.compare_exchange_strong(
		expected,
		status::active,
		std::memory_order_relaxed);

	if (swapped) {
		m_approx_size.fetch_sub(1, std::memory_order_relaxed);
	}

	return swapped;
}

size_t idle_worker_set::find_idle_worker(size_t caller_index) noexcept {
	if (m_approx_size.load(std::memory_order_relaxed) <= 0) {
		return static_cast<size_t>(-1);
	}

	const auto starting_pos = s_tl_thread_pool_data.randomizer() % m_size;
	for (size_t i = 0; i < m_size; i++) {
		const auto index = (starting_pos + i) % m_size;
		if (index == caller_index) {
			continue;
		}

		if (try_acquire_flag(index)) {
			return index;
		}
	}

	return static_cast<size_t>(-1);
}

void idle_worker_set::find_idle_workers(
	size_t caller_index,
	std::vector<size_t>& result_buffer,
	size_t max_count) noexcept {
	assert(result_buffer.capacity() >= max_count);

	if (m_approx_size.load(std::memory_order_relaxed) <= 0) {
		return;
	}

	const auto starting_pos = s_tl_thread_pool_data.randomizer() % m_size;
	size_t count = 0;

	for (size_t i = 0; (i < m_size) && (count < max_count); i++) {
		const auto index = (starting_pos + i) % m_size;
		if (index == caller_index) {
			continue;
		}

		if (try_acquire_flag(index)) {
			result_buffer.emplace_back(index);
			++count;
		}
	}
}

thread_pool_worker::thread_pool_worker(
	thread_pool_executor& parent_pool,
	size_t index,
	size_t pool_size,
	std::chrono::seconds max_idle_time) :
	m_atomic_abort(false),
	m_parent_pool(parent_pool),
	m_index(index),
	m_pool_size(pool_size),
	m_max_idle_time(max_idle_time),
	m_worker_name(details::make_executor_worker_name(parent_pool.name)),
	m_status(status::idle),
	m_abort(false) {
	m_idle_worker_list.reserve(pool_size);
}

thread_pool_worker::thread_pool_worker(thread_pool_worker&& rhs) noexcept :
	m_parent_pool(rhs.m_parent_pool),
	m_index(rhs.m_index),
	m_pool_size(rhs.m_pool_size),
	m_max_idle_time(rhs.m_max_idle_time) {
	std::abort(); //shouldn't be called
}

thread_pool_worker::~thread_pool_worker() noexcept {
	assert(m_status == status::idle);
	assert(!m_thread.joinable());
}

void thread_pool_worker::balance_work() {
	const auto task_count = m_private_queue.size();
	if (task_count == 0) {
		return;
	}

	//we assume all threads but us are idle
	const auto idle_worker_count = std::min(m_pool_size - 1, task_count);
	if (idle_worker_count == 0) {
		return; // a thread-pool with a single thread
	}

	m_parent_pool.find_idle_workers(m_index, m_idle_worker_list, idle_worker_count);
	if (m_idle_worker_list.empty()) {
		return;
	}

	for (auto idle_worker_index : m_idle_worker_list) {
		assert(idle_worker_index != m_index);
		assert(idle_worker_index < m_pool_size);

		const auto task = m_private_queue.front();
		m_private_queue.pop_front();
		m_parent_pool.worker_at(idle_worker_index).enqueue_foreign(task);
	}

	m_idle_worker_list.clear();
}


bool thread_pool_worker::wait_for_task(std::unique_lock<std::mutex>& lock) noexcept {
	assert(lock.owns_lock());

	m_parent_pool.mark_worker_idle(m_index);
	m_status = status::waiting;

	const auto timeout = !m_condition.wait_for(lock, m_max_idle_time, [this] {
		return !m_public_queue.empty() || m_abort;
	});

	if (timeout || m_abort) {
		m_status = status::idle;
		lock.unlock();
		return false;
	}

	assert(!m_public_queue.empty());
	m_status = status::working;
	m_parent_pool.mark_worker_active(m_index);
	return true;
}

bool thread_pool_worker::drain_queue_impl() {
	while (!m_private_queue.empty()) {
		auto task = m_private_queue.back();
		m_private_queue.pop_back();

		balance_work();

		if (m_atomic_abort.load(std::memory_order_relaxed)) {
			std::unique_lock<std::mutex> lock(m_lock);
			m_status = status::idle;
			return false;
		}

		task();
	}

	return true;
}

bool thread_pool_worker::drain_queue() {
	assert(m_private_queue.empty());
	m_private_queue = {};

	std::unique_lock<std::mutex> lock(m_lock);
	if (m_public_queue.empty() && m_abort == false) {
		if (!wait_for_task(lock)) {
			return false;
		}
	}

	assert(lock.owns_lock());

	if (m_abort) {
		m_status = status::idle;
		return false;
	}

	m_private_queue = std::move(m_public_queue);
	lock.unlock();

	return drain_queue_impl();
}

void thread_pool_worker::work_loop() noexcept {
	s_tl_thread_pool_data.this_worker = this;
	s_tl_thread_pool_data.this_thread_index = m_index;

	while (true) {
		if (!drain_queue()) {
			return;
		}
	}
}

void thread_pool_worker::destroy_tasks() noexcept {
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

void thread_pool_worker::ensure_worker_active(std::unique_lock<std::mutex>& lock) {
	assert(lock.owns_lock());
	const auto status = m_status;

	switch (status) {
		case status::working: {
			lock.unlock();
			return;
		}

		case status::waiting: {
			lock.unlock();
			m_condition.notify_one();
			return;
		}

		case status::idle: {
			auto stale_worker = std::move(m_thread);
			m_thread = thread(m_worker_name, [this] {
				work_loop();
			});

			m_status = status::working;
			lock.unlock();

			if (stale_worker.joinable()) {
				stale_worker.join();
			}
		}
	}
}

void thread_pool_worker::enqueue_foreign(std::experimental::coroutine_handle<> task) {
	std::unique_lock<std::mutex> lock(m_lock);
	if (m_abort) {
		throw_executor_shutdown_exception(m_parent_pool.name);
	}

	m_public_queue.emplace_back(task);
	ensure_worker_active(lock);
}

void thread_pool_worker::enqueue_foreign(std::span<std::experimental::coroutine_handle<>> tasks) {
	std::unique_lock<std::mutex> lock(m_lock);
	if (m_abort) {
		throw_executor_shutdown_exception(m_parent_pool.name);
	}

	m_public_queue.insert(m_public_queue.end(), tasks.begin(), tasks.end());
	ensure_worker_active(lock);
}

void thread_pool_worker::enqueue_local(std::experimental::coroutine_handle<> task) {
	if (m_atomic_abort.load(std::memory_order_relaxed)) {
		throw_executor_shutdown_exception(m_parent_pool.name);
	}

	m_private_queue.emplace_back(task);
}

void thread_pool_worker::enqueue_local(std::span<std::experimental::coroutine_handle<>> tasks) {
	if (m_atomic_abort.load(std::memory_order_relaxed)) {
		throw_executor_shutdown_exception(m_parent_pool.name);
	}

	m_private_queue.insert(m_private_queue.end(), tasks.begin(), tasks.end());
}

void thread_pool_worker::abort() noexcept {
	m_atomic_abort.store(true, std::memory_order_relaxed);

	{
		std::unique_lock<std::mutex> lock(m_lock);
		m_abort = true;
	}

	m_condition.notify_all();
}

void thread_pool_worker::join() noexcept {
	if (m_thread.joinable()) {
		m_thread.join();
	}

	destroy_tasks();
}

thread_pool_executor::thread_pool_executor(std::string_view name, size_t size, std::chrono::seconds max_idle_time) :
	executor(name),
	m_round_robin_cursor(0),
	m_idle_workers(size),
	m_abort(false) {
	m_workers.reserve(size);

	std::srand(static_cast<unsigned int>(std::time(nullptr)));

	for (size_t i = 0; i < size; i++) {
		m_workers.emplace_back(*this, i, size, max_idle_time);
	}

	for (size_t i = 0; i < size; i++) {
		m_idle_workers.set_idle(i);
	}
}

thread_pool_executor::~thread_pool_executor() noexcept {}

void thread_pool_executor::find_idle_workers(size_t caller_index, std::vector<size_t>& buffer, size_t max_count) noexcept {
	m_idle_workers.find_idle_workers(caller_index, buffer, max_count);
}

void thread_pool_executor::mark_worker_idle(size_t index) noexcept {
	assert(index < m_workers.size());
	m_idle_workers.set_idle(index);
}

void thread_pool_executor::mark_worker_active(size_t index) noexcept {
	assert(index < m_workers.size());
	m_idle_workers.set_active(index);
}

void thread_pool_executor::enqueue(std::experimental::coroutine_handle<> task) {
	const auto idle_worker_pos = m_idle_workers.find_idle_worker(details::s_tl_thread_pool_data.this_thread_index);
	if (idle_worker_pos != static_cast<size_t>(-1)) {
		return m_workers[idle_worker_pos].enqueue_foreign(task);
	}

	if (details::s_tl_thread_pool_data.this_worker != nullptr) {
		return details::s_tl_thread_pool_data.this_worker->enqueue_local(task);
	}

	const auto next_worker = m_round_robin_cursor.fetch_add(1, std::memory_order_relaxed) % m_workers.size();
	m_workers[next_worker].enqueue_foreign(task);
}

void thread_pool_executor::enqueue(std::span<std::experimental::coroutine_handle<>> tasks) {
	if (details::s_tl_thread_pool_data.this_worker != nullptr) {
		return details::s_tl_thread_pool_data.this_worker->enqueue_local(tasks);
	}

	if (tasks.size() < m_workers.size() * 2) {
		for (auto task : tasks) {
			enqueue(task);
		}

		return;
	}

	const auto approx_bulk_size = static_cast<float>(tasks.size()) / static_cast<float>(m_workers.size());
	const auto bulk_size = static_cast<size_t>(std::ceil(approx_bulk_size));

	size_t worker_index = 0;
	auto cursor = tasks.data();
	const auto absolute_end = tasks.data() + tasks.size();

	while (cursor < absolute_end) {
		auto end = (cursor + bulk_size > absolute_end) ? absolute_end : (cursor + bulk_size);
		std::span<std::experimental::coroutine_handle<>> range = { cursor, end };
		m_workers[worker_index].enqueue_foreign(range);
		cursor += bulk_size;
		++worker_index;
	}
}

int thread_pool_executor::max_concurrency_level() const noexcept {
	return static_cast<int>(m_workers.size());
}

bool thread_pool_executor::shutdown_requested() const noexcept {
	return m_abort.load(std::memory_order_relaxed);
}

void concurrencpp::thread_pool_executor::shutdown() noexcept {
	m_abort.store(true, std::memory_order_relaxed);

	for (auto& worker : m_workers) {
		worker.abort();
	}

	std::this_thread::yield();

	for (auto& worker : m_workers) {
		worker.join();
	}
}
