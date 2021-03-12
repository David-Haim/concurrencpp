#include "concurrencpp/results/impl/shared_result_state.h"

using concurrencpp::details::shared_result_state_base;

/*
 * shared_await_via_context
 */

concurrencpp::details::shared_await_via_context::shared_await_via_context(
    const std::shared_ptr<concurrencpp::executor>& executor) noexcept :
    await_context(executor) {}

/*
 * shared_result_state_base
 */

void shared_result_state_base::await_impl(std::unique_lock<std::shared_mutex>& write_lock, shared_await_context& awaiter) noexcept {
    assert(write_lock.owns_lock());

    if (m_awaiters == nullptr) {
        m_awaiters = &awaiter;
        return;
    }

    awaiter.next = m_awaiters;
    m_awaiters = &awaiter;
}

void shared_result_state_base::await_via_impl(std::unique_lock<std::shared_mutex>& write_lock,
                                              shared_await_via_context& awaiter) noexcept {
    if (m_via_awaiters == nullptr) {
        m_via_awaiters = &awaiter;
        return;
    }

    awaiter.next = m_via_awaiters;
    m_via_awaiters = &awaiter;
}

void shared_result_state_base::wait_impl(std::unique_lock<std::shared_mutex>& lock) noexcept {
    assert(lock.owns_lock());
    m_condition.wait(lock, [this] {
        return m_ready;
    });
}

bool shared_result_state_base::wait_for_impl(std::unique_lock<std::shared_mutex>& write_lock, std::chrono::milliseconds ms) noexcept {
    assert(write_lock.owns_lock());
    return m_condition.wait_for(write_lock, ms, [this] {
        return m_ready;
    });
}

void shared_result_state_base::notify_all(std::unique_lock<std::shared_mutex>& write_lock) noexcept {
    assert(write_lock.owns_lock());

    m_ready = true;
    shared_await_context* awaiters = std::exchange(m_awaiters, nullptr);
    shared_await_via_context* via_awaiters = std::exchange(m_via_awaiters, nullptr);
    write_lock.unlock();

    // unblock waiters
    m_condition.notify_all();

    // unblock awaiters that want to be resumed by an executor
    while (via_awaiters != nullptr) {
        const auto next = via_awaiters->next;
        via_awaiters->await_context();
        via_awaiters = next;
    }

    // unblock synchronous awaiters
    while (awaiters != nullptr) {
        const auto next = awaiters->next;
        awaiters->caller_handle();
        awaiters = next;
    }
}