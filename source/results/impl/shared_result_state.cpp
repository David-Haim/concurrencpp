#include "concurrencpp/results/impl/shared_result_state.h"

using concurrencpp::details::shared_result_state_base;

void shared_result_state_base::await_impl(std::unique_lock<std::mutex>& lock, shared_await_context& awaiter) noexcept {
    assert(lock.owns_lock());

    if (m_awaiters == nullptr) {
        m_awaiters = &awaiter;
        return;
    }

    awaiter.next = m_awaiters;
    m_awaiters = &awaiter;
}

void shared_result_state_base::wait_impl(std::unique_lock<std::mutex>& lock) {
    assert(lock.owns_lock());

    if (!m_condition.has_value()) {
        m_condition.emplace();
    }

    m_condition.value().wait(lock, [this] {
        return m_ready.load(std::memory_order_relaxed);
    });
}

bool shared_result_state_base::wait_for_impl(std::unique_lock<std::mutex>& lock, std::chrono::milliseconds ms) {
    assert(lock.owns_lock());

    if (!m_condition.has_value()) {
        m_condition.emplace();
    }

    return m_condition.value().wait_for(lock, ms, [this] {
        return m_ready.load(std::memory_order_relaxed);
    });
}

void shared_result_state_base::complete_producer() {
    shared_await_context* awaiters;

    {
        std::unique_lock<std::mutex> lock(m_lock);
        awaiters = std::exchange(m_awaiters, nullptr);
        m_ready.store(true, std::memory_order_release);

        if (m_condition.has_value()) {
            m_condition.value().notify_all();
        }
    }

    while (awaiters != nullptr) {
        const auto next = awaiters->next;
        awaiters->caller_handle();
        awaiters = next;
    }
}

bool shared_result_state_base::await(shared_await_context& awaiter) {
    if (m_ready.load(std::memory_order_acquire)) {
        return false;
    }

    {
        std::unique_lock<std::mutex> lock(m_lock);
        if (m_ready.load(std::memory_order_acquire)) {
            return false;
        }

        await_impl(lock, awaiter);
    }

    return true;
}

void shared_result_state_base::wait() {
    if (m_ready.load(std::memory_order_acquire)) {
        return;
    }

    {
        std::unique_lock<std::mutex> lock(m_lock);
        if (m_ready.load(std::memory_order_acquire)) {
            return;
        }

        wait_impl(lock);
    }
}
