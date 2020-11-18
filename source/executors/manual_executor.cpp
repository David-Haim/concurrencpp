#include "concurrencpp/executors/manual_executor.h"
#include "concurrencpp/executors/constants.h"

using concurrencpp::manual_executor;

manual_executor::manual_executor() :
    derivable_executor<concurrencpp::manual_executor>(details::consts::k_manual_executor_name), m_abort(false), m_atomic_abort(false) {}

void manual_executor::enqueue(concurrencpp::task task) {
    std::unique_lock<decltype(m_lock)> lock(m_lock);
    if (m_abort) {
        details::throw_executor_shutdown_exception(name);
    }

    m_tasks.emplace_back(std::move(task));
    lock.unlock();
    m_condition.notify_all();
}

void manual_executor::enqueue(std::span<concurrencpp::task> tasks) {
    std::unique_lock<decltype(m_lock)> lock(m_lock);
    if (m_abort) {
        details::throw_executor_shutdown_exception(name);
    }

    m_tasks.insert(m_tasks.end(), std::make_move_iterator(tasks.begin()), std::make_move_iterator(tasks.end()));
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

    auto task = std::move(m_tasks.front());
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

    auto task = std::move(m_tasks.front());
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

bool manual_executor::wait_for_task(std::chrono::milliseconds max_waiting_time) {
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

    decltype(m_tasks) tasks;

    {
        std::unique_lock<decltype(m_lock)> lock(m_lock);
        m_abort = true;
        tasks = std::move(m_tasks);
    }

    m_condition.notify_all();
}

bool manual_executor::shutdown_requested() const noexcept {
    return m_atomic_abort.load(std::memory_order_relaxed);
}
