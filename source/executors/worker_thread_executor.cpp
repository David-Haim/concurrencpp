#include "concurrencpp/executors/worker_thread_executor.h"
#include "concurrencpp/executors/constants.h"

namespace concurrencpp::details {
    static thread_local worker_thread_executor* s_tl_this_worker = nullptr;
}

using concurrencpp::worker_thread_executor;

worker_thread_executor::worker_thread_executor() :
    derivable_executor<concurrencpp::worker_thread_executor>(details::consts::k_worker_thread_executor_name), m_private_atomic_abort(false), m_abort(false),
    m_atomic_abort(false) {
    m_thread = details::thread(details::make_executor_worker_name(name), [this] {
        work_loop();
    });
}

bool worker_thread_executor::drain_queue_impl() {
    while (!m_private_queue.empty()) {
        auto task = std::move(m_private_queue.front());
        m_private_queue.pop_front();

        if (m_private_atomic_abort.load(std::memory_order_relaxed)) {
            return false;
        }

        task();
    }

    return true;
}

bool worker_thread_executor::drain_queue() {
    std::unique_lock<decltype(m_lock)> lock(m_lock);
    m_condition.wait(lock, [this] {
        return !m_public_queue.empty() || m_abort;
    });

    if (m_abort) {
        return false;
    }

    assert(m_private_queue.empty());
    std::swap(m_private_queue, m_public_queue);  // reuse underlying allocations.
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

void worker_thread_executor::enqueue_local(concurrencpp::task& task) {
    if (m_private_atomic_abort.load(std::memory_order_relaxed)) {
        details::throw_executor_shutdown_exception(name);
    }

    m_private_queue.emplace_back(std::move(task));
}

void worker_thread_executor::enqueue_local(std::span<concurrencpp::task> tasks) {
    if (m_private_atomic_abort.load(std::memory_order_relaxed)) {
        details::throw_executor_shutdown_exception(name);
    }

    m_private_queue.insert(m_private_queue.end(), std::make_move_iterator(tasks.begin()), std::make_move_iterator(tasks.end()));
}

void worker_thread_executor::enqueue_foreign(concurrencpp::task& task) {
    std::unique_lock<decltype(m_lock)> lock(m_lock);
    if (m_abort) {
        details::throw_executor_shutdown_exception(name);
    }

    m_public_queue.emplace_back(std::move(task));
    lock.unlock();

    m_condition.notify_one();
}

void worker_thread_executor::enqueue_foreign(std::span<concurrencpp::task> tasks) {
    std::unique_lock<decltype(m_lock)> lock(m_lock);
    if (m_abort) {
        details::throw_executor_shutdown_exception(name);
    }

    m_public_queue.insert(m_public_queue.end(), std::make_move_iterator(tasks.begin()), std::make_move_iterator(tasks.end()));
    lock.unlock();

    m_condition.notify_one();
}

void worker_thread_executor::enqueue(concurrencpp::task task) {
    if (details::s_tl_this_worker == this) {
        return enqueue_local(task);
    }

    enqueue_foreign(task);
}

void worker_thread_executor::enqueue(std::span<concurrencpp::task> tasks) {
    if (details::s_tl_this_worker == this) {
        return enqueue_local(tasks);
    }

    enqueue_foreign(tasks);
}

int worker_thread_executor::max_concurrency_level() const noexcept {
    return details::consts::k_worker_thread_max_concurrency_level;
}

bool worker_thread_executor::shutdown_requested() const noexcept {
    return m_atomic_abort.load(std::memory_order_relaxed);
}

void worker_thread_executor::shutdown() noexcept {
    const auto abort = m_atomic_abort.exchange(true, std::memory_order_relaxed);
    if (abort) {
        return;  // shutdown had been called before.
    }

    assert(m_private_atomic_abort.load(std::memory_order_relaxed) == false);
    m_private_atomic_abort.store(true, std::memory_order_relaxed);

    decltype(m_private_queue) private_queue;
    decltype(m_public_queue) public_queue;

    {
        std::unique_lock<std::mutex> lock(m_lock);
        m_abort = true;

        private_queue = std::move(m_private_queue);
        public_queue = std::move(m_public_queue);
    }

    m_condition.notify_all();

    private_queue.clear();
    public_queue.clear();

    if (m_thread.joinable()) {
        m_thread.join();
    }
}
