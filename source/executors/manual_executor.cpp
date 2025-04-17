#include "concurrencpp/utils/throw_helper.h"
#include "concurrencpp/executors/constants.h"
#include "concurrencpp/executors/manual_executor.h"

using concurrencpp::manual_executor;

manual_executor::manual_executor() :
    derivable_executor<concurrencpp::manual_executor>(details::consts::k_manual_executor_name), m_abort(false), m_atomic_abort(false) {
}

void manual_executor::enqueue(concurrencpp::task task) {
    std::unique_lock<decltype(m_lock)> lock(m_lock);
    if (m_abort) {
        details::throw_helper::throw_worker_shutdown_exception(name, "enqueue");
    }

    m_tasks.emplace_back(std::move(task));
    lock.unlock();

    m_condition.notify_all();
}

void manual_executor::enqueue(std::span<concurrencpp::task> tasks) {
    std::unique_lock<decltype(m_lock)> lock(m_lock);
    if (m_abort) {
        details::throw_helper::throw_worker_shutdown_exception(name, "enqueue");
    }

    m_tasks.insert(m_tasks.end(), std::make_move_iterator(tasks.begin()), std::make_move_iterator(tasks.end()));
    lock.unlock();

    m_condition.notify_all();
}

int manual_executor::max_concurrency_level() const noexcept {
    return details::consts::k_manual_executor_max_concurrency_level;
}

size_t manual_executor::size() const {
    std::unique_lock<std::mutex> lock(m_lock);
    return m_tasks.size();
}

bool manual_executor::empty() const {
    return size() == 0;
}

size_t manual_executor::loop_impl(size_t max_count, const char* calling_method) {
    if (max_count == 0) {
        return 0;
    }

    size_t executed = 0;

    while (true) {
        if (executed == max_count) {
            break;
        }

        std::unique_lock<decltype(m_lock)> lock(m_lock);
        if (m_abort) {
            break;
        }

        if (m_tasks.empty()) {
            break;
        }

        auto task = std::move(m_tasks.front());
        m_tasks.pop_front();
        lock.unlock();

        task();
        ++executed;
    }

    if (shutdown_requested()) {
        details::throw_helper::throw_worker_shutdown_exception(name, calling_method);
    }

    return executed;
}

size_t manual_executor::loop_until_impl(size_t max_count,
                                        std::chrono::time_point<std::chrono::system_clock> deadline,
                                        const char* calling_method) {
    if (max_count == 0) {
        return 0;
    }

    size_t executed = 0;
    deadline += std::chrono::milliseconds(1);

    while (true) {
        if (executed == max_count) {
            break;
        }

        const auto now = std::chrono::system_clock::now();
        if (now >= deadline) {
            break;
        }

        std::unique_lock<decltype(m_lock)> lock(m_lock);
        const auto found_task = m_condition.wait_until(lock, deadline, [this] {
            return !m_tasks.empty() || m_abort;
        });

        if (m_abort) {
            break;
        }

        if (!found_task) {
            break;
        }

        assert(!m_tasks.empty());
        auto task = std::move(m_tasks.front());
        m_tasks.pop_front();
        lock.unlock();

        task();
        ++executed;
    }

    if (shutdown_requested()) {
        details::throw_helper::throw_worker_shutdown_exception(name, calling_method);
    }

    return executed;
}

void manual_executor::wait_for_tasks_impl(size_t count, const char* calling_method) {
    if (count == 0) {
        if (shutdown_requested()) {
            details::throw_helper::throw_worker_shutdown_exception(name, calling_method);
        }
        return;
    }

    std::unique_lock<decltype(m_lock)> lock(m_lock);
    m_condition.wait(lock, [this, count] {
        return (m_tasks.size() >= count) || m_abort;
    });

    if (m_abort) {
        details::throw_helper::throw_worker_shutdown_exception(name, calling_method);
    }

    assert(m_tasks.size() >= count);
}

size_t manual_executor::wait_for_tasks_impl(size_t count,
                                            std::chrono::time_point<std::chrono::system_clock> deadline,
                                            const char* calling_method) {
    deadline += std::chrono::milliseconds(1);

    std::unique_lock<decltype(m_lock)> lock(m_lock);
    m_condition.wait_until(lock, deadline, [this, count] {
        return (m_tasks.size() >= count) || m_abort;
    });

    if (m_abort) {
        details::throw_helper::throw_worker_shutdown_exception(name, calling_method);
    }

    return m_tasks.size();
}

bool manual_executor::loop_once() {
    return loop_impl(1, "loop_once") != 0;
}

bool manual_executor::loop_once_for(std::chrono::milliseconds max_waiting_time) {
    if (max_waiting_time == std::chrono::milliseconds(0)) {
        return loop_impl(1, "loop_once_for") != 0;
    }

    return loop_until_impl(1, time_point_from_now(max_waiting_time), "loop_once_for");
}

size_t manual_executor::loop(size_t max_count) {
    return loop_impl(max_count, "loop");
}

size_t manual_executor::loop_for(size_t max_count, std::chrono::milliseconds max_waiting_time) {
    if (max_count == 0) {
        return 0;
    }

    if (max_waiting_time == std::chrono::milliseconds(0)) {
        return loop_impl(max_count, "loop_for");
    }

    return loop_until_impl(max_count, time_point_from_now(max_waiting_time), "loop_for");
}

size_t manual_executor::clear() {
    std::unique_lock<decltype(m_lock)> lock(m_lock);
    if (m_abort) {
        details::throw_helper::throw_worker_shutdown_exception(name, "clear");
    }

    const auto tasks = std::move(m_tasks);
    lock.unlock();
    return tasks.size();
}

void manual_executor::wait_for_task() {
    wait_for_tasks_impl(1, "wait_for_task");
}

bool manual_executor::wait_for_task_for(std::chrono::milliseconds max_waiting_time) {
    return wait_for_tasks_impl(1, time_point_from_now(max_waiting_time), "wait_for_task_for") == 1;
}

void manual_executor::wait_for_tasks(size_t count) {
    wait_for_tasks_impl(count, "wait_for_tasks");
}

size_t manual_executor::wait_for_tasks_for(size_t count, std::chrono::milliseconds max_waiting_time) {
    return wait_for_tasks_impl(count, time_point_from_now(max_waiting_time), "wait_for_tasks_for");
}

void manual_executor::shutdown() {
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

    tasks.clear();
}

bool manual_executor::shutdown_requested() const {
    return m_atomic_abort.load(std::memory_order_relaxed);
}