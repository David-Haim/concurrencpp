#include "manual_executor.h"
#include "constants.h"

using concurrencpp::manual_executor;

void manual_executor::destroy_tasks(
    std::unique_lock<std::mutex>& lock) noexcept {
  assert(lock.owns_lock());
  (void)lock;

  for (auto task : m_tasks) {
    task.destroy();
  }

  m_tasks.clear();
}

manual_executor::manual_executor() :
    derivable_executor<concurrencpp::manual_executor>(
        details::consts::k_manual_executor_name),
    m_abort(false), m_atomic_abort(false) {}

void manual_executor::enqueue(std::experimental::coroutine_handle<> task) {
  std::unique_lock<decltype(m_lock)> lock(m_lock);
  if (m_abort) {
    details::throw_executor_shutdown_exception(name);
  }

  m_tasks.emplace_back(task);
  lock.unlock();
  m_condition.notify_all();
}

void manual_executor::enqueue(
    std::span<std::experimental::coroutine_handle<>> tasks) {
  std::unique_lock<decltype(m_lock)> lock(m_lock);
  if (m_abort) {
    details::throw_executor_shutdown_exception(name);
  }

  m_tasks.insert(m_tasks.end(), tasks.begin(), tasks.end());
  lock.unlock();

  m_condition.notify_all();
}

int manual_executor::max_concurrency_level() const noexcept {
  return details::consts::k_manual_executor_max_concurrency_level;
}

size_t manual_executor::size() const noexcept {
  std::unique_lock<decltype(m_lock)> lock(m_lock);
  return m_tasks.size();
}

bool manual_executor::empty() const noexcept {
  return size() == 0;
}

bool manual_executor::loop_once() {
  std::unique_lock<decltype(m_lock)> lock(m_lock);
  if (m_abort) {
    details::throw_executor_shutdown_exception(name);
  }

  if (m_tasks.empty()) {
    return false;
  }

  auto task = m_tasks.front();
  m_tasks.pop_front();
  lock.unlock();

  task();
  return true;
}

bool manual_executor::loop_once(std::chrono::milliseconds max_waiting_time) {
  std::unique_lock<decltype(m_lock)> lock(m_lock);
  m_condition.wait_for(lock, max_waiting_time, [this] {
    return !m_tasks.empty() || m_abort;
  });

  if (m_abort) {
    details::throw_executor_shutdown_exception(name);
  }

  if (m_tasks.empty()) {
    return false;
  }

  auto task = m_tasks.front();
  m_tasks.pop_front();
  lock.unlock();

  task();
  return true;
}

size_t manual_executor::loop(size_t max_count) {
  size_t executed = 0;
  for (; executed < max_count; ++executed) {
    if (!loop_once()) {
      break;
    }
  }

  return executed;
}

size_t manual_executor::clear() noexcept {
  std::unique_lock<decltype(m_lock)> lock(m_lock);
  auto tasks = std::move(m_tasks);
  lock.unlock();

  for (auto task : tasks) {
    task.destroy();
  }

  return tasks.size();
}

void manual_executor::wait_for_task() {
  std::unique_lock<decltype(m_lock)> lock(m_lock);
  m_condition.wait(lock, [this] {
    return !m_tasks.empty() || m_abort;
  });

  if (m_abort) {
    details::throw_executor_shutdown_exception(name);
  }
}

bool manual_executor::wait_for_task(
    std::chrono::milliseconds max_waiting_time) {
  std::unique_lock<decltype(m_lock)> lock(m_lock);
  const auto res = m_condition.wait_for(lock, max_waiting_time, [this] {
    return !m_tasks.empty() || m_abort;
  });

  if (!res) {
    assert(!m_abort && m_tasks.empty());
    return false;
  }

  if (!m_abort) {
    assert(!m_tasks.empty());
    return true;
  }

  details::throw_executor_shutdown_exception(name);
}

void manual_executor::shutdown() noexcept {
  const auto abort = m_atomic_abort.exchange(true, std::memory_order_relaxed);
  if (abort) {
    return;  // shutdown had been called before.
  }

  {
    std::unique_lock<decltype(m_lock)> lock(m_lock);
    m_abort = true;

    destroy_tasks(lock);
  }

  m_condition.notify_all();
}

bool manual_executor::shutdown_requested() const noexcept {
  return m_atomic_abort.load(std::memory_order_relaxed);
}