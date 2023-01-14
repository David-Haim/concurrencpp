#include "concurrencpp/results/impl/shared_result_state.h"

using concurrencpp::details::shared_result_state_base;

bool shared_result_state_base::await(shared_await_context& awaiter) noexcept {
    while (true) {
        auto awaiter_before = m_awaiters.load(std::memory_order_acquire);
        if (awaiter_before == k_result_ready) {
            return false;
        }

        awaiter.next = awaiter_before;
        const auto swapped = m_awaiters.compare_exchange_weak(awaiter_before, &awaiter, std::memory_order_acq_rel);
        if (swapped) {
            return true;
        }
    }
}

void shared_result_state_base::on_result_finished() noexcept {
    auto awaiters = m_awaiters.exchange(k_result_ready, std::memory_order_acq_rel);

    shared_await_context* current = awaiters;
    shared_await_context *prev = nullptr, *next = nullptr;

    while (current != nullptr) {
        next = current->next;
        current->next = prev;
        prev = current;
        current = next;
    }

    awaiters = prev;

    while (awaiters != nullptr) {
        assert(static_cast<bool>(awaiters->caller_handle));
        awaiters->caller_handle();
        awaiters = awaiters->next;
    }

    // theoretically buggish, practically there's no way that we'll have more than (2^32)/2 waiters..
    m_semaphore.release(k_max_waiters / 2);
}