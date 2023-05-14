#include "concurrencpp/results/impl/shared_result_state.h"

using concurrencpp::details::shared_result_state_base;

concurrencpp::details::shared_await_context* shared_result_state_base::result_ready_constant() noexcept {
    return reinterpret_cast<shared_await_context*>(-1);
}

concurrencpp::result_status concurrencpp::details::shared_result_state_base::status() const noexcept {
    return m_status.load(std::memory_order_acquire);
}

bool shared_result_state_base::await(shared_await_context& awaiter) noexcept {
    while (true) {
        auto awaiter_before = m_awaiters.load(std::memory_order_acquire);
        if (awaiter_before == result_ready_constant()) {
            return false;
        }

        awaiter.next = awaiter_before;
        const auto swapped = m_awaiters.compare_exchange_weak(awaiter_before, &awaiter, std::memory_order_acq_rel);
        if (swapped) {
            return true;
        }
    }
}

void concurrencpp::details::shared_result_state_base::wait() noexcept {
    if (status() == result_status::idle) {
        m_status.wait(result_status::idle, std::memory_order_acquire);
    }
}
