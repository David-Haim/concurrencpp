#include "concurrencpp/executors/constants.h"
#include "concurrencpp/executors/thread_executor.h"

using concurrencpp::thread_executor;

thread_executor::thread_executor() : executor(details::consts::k_thread_executor_name), m_abort(false), m_atomic_abort(false) {}

thread_executor::~thread_executor() noexcept {
    assert(m_workers.empty());
    assert(m_last_retired.empty());
}

void thread_executor::enqueue(concurrencpp::task& task) {
    std::unique_lock<std::mutex> lock(m_lock);
    if (m_abort) {
        details::throw_runtime_shutdown_exception(name);
    }

    auto& new_thread = m_workers.emplace_front();
    new_thread = details::thread(details::make_executor_worker_name(name), [this, self_it = m_workers.begin(), &task]() mutable {
        task.resume();
        retire_worker(self_it);
    });
}

int thread_executor::max_concurrency_level() const noexcept {
    return details::consts::k_thread_executor_max_concurrency_level;
}

bool thread_executor::shutdown_requested() const {
    return m_atomic_abort.load(std::memory_order_relaxed);
}

void thread_executor::shutdown() {
    const auto abort = m_atomic_abort.exchange(true, std::memory_order_relaxed);
    if (abort) {
        return;  // shutdown had been called before.
    }

    std::unique_lock<std::mutex> lock(m_lock);
    m_abort = true;
    m_condition.wait(lock, [this] {
        return m_workers.empty();
    });

    if (m_last_retired.empty()) {
        return;
    }

    assert(m_last_retired.size() == 1);
    m_last_retired.front().join();
    m_last_retired.clear();
}

void thread_executor::retire_worker(std::list<details::thread>::iterator it) {
    std::unique_lock<std::mutex> lock(m_lock);
    auto last_retired = std::move(m_last_retired);
    m_last_retired.splice(m_last_retired.begin(), m_workers, it);

    lock.unlock();
    m_condition.notify_one();

    if (last_retired.empty()) {
        return;
    }

    assert(last_retired.size() == 1);
    last_retired.front().join();
}