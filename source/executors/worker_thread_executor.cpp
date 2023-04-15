#include "concurrencpp/executors/constants.h"
#include "concurrencpp/executors/worker_thread_executor.h"

namespace concurrencpp::details {
    static thread_local worker_thread_executor* s_tl_this_worker = nullptr;
}

using concurrencpp::worker_thread_executor;

worker_thread_executor::worker_thread_executor() :
    executor(details::consts::k_worker_thread_executor_name), m_private_atomic_abort(false), m_semaphore(0), m_atomic_abort(false),
    m_abort(false) {
    m_thread = details::thread(make_executor_worker_name(), [this] {
        work_loop();
    });
}

bool worker_thread_executor::drain_queue_impl() {
    while (!m_private_queue.empty()) {
        if (m_private_atomic_abort.load(std::memory_order_relaxed)) {
            return false;
        }

        auto& task = m_private_queue.pop_front();
        task.resume();
    }

    return true;
}

void worker_thread_executor::wait_for_task(std::unique_lock<std::mutex>& lock) {
    assert(lock.owns_lock());
    if (!m_public_queue.empty() || m_abort) {
        return;
    }

    while (true) {
        lock.unlock();

        m_semaphore.acquire();

        lock.lock();
        if (!m_public_queue.empty() || m_abort) {
            break;
        }
    }
}

bool worker_thread_executor::drain_queue() {
    std::unique_lock<std::mutex> lock(m_lock);
    wait_for_task(lock);

    assert(lock.owns_lock());
    assert(!m_public_queue.empty() || m_abort);

    if (m_abort) {
        return false;
    }

    assert(m_private_queue.empty());
    std::swap(m_private_queue, m_public_queue);  // reuse underlying allocations.
    lock.unlock();

    return drain_queue_impl();
}

void worker_thread_executor::work_loop() {
    details::s_tl_this_worker = this;

    while (true) {
        if (!drain_queue()) {
            return;
        }
    }
}

void worker_thread_executor::enqueue_local(concurrencpp::task& task) {
    if (m_private_atomic_abort.load(std::memory_order_relaxed)) {
        throw_runtime_shutdown_exception();
    }

    m_private_queue.push_back(task);
}

void worker_thread_executor::enqueue_foreign(concurrencpp::task& task) {
    std::unique_lock<std::mutex> lock(m_lock);
    if (m_abort) {
        throw_runtime_shutdown_exception();
    }

    const auto is_empty = m_public_queue.empty();
    m_public_queue.push_front(task);
    lock.unlock();

    if (is_empty) {
        m_semaphore.release();
    }
}

void worker_thread_executor::enqueue(concurrencpp::task& task) {
    if (details::s_tl_this_worker == this) {
        return enqueue_local(task);
    }

    enqueue_foreign(task);
}

int worker_thread_executor::max_concurrency_level() const noexcept {
    return details::consts::k_worker_thread_max_concurrency_level;
}

bool worker_thread_executor::shutdown_requested() const {
    return m_atomic_abort.load(std::memory_order_relaxed);
}

void worker_thread_executor::shutdown() {
    const auto abort = m_atomic_abort.exchange(true, std::memory_order_relaxed);
    if (abort) {
        return;  // shutdown had been called before.
    }

    {
        std::unique_lock<std::mutex> lock(m_lock);
        m_abort = true;
    }

    m_private_atomic_abort.store(true, std::memory_order_relaxed);
    m_semaphore.release();

    if (m_thread.joinable()) {
        m_thread.join();
    }

    decltype(m_private_queue) private_queue;
    decltype(m_public_queue) public_queue;

    {
        std::unique_lock<std::mutex> lock(m_lock);
        private_queue = std::move(m_private_queue);
        public_queue = std::move(m_public_queue);
    }

    while (!public_queue.empty()) {
        auto& task = public_queue.pop_front();
        task.interrupt();
    }

    while (!private_queue.empty()) {
        auto& task = private_queue.pop_front();
        task.interrupt();
    }
}
