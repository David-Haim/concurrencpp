#include "utils/wait_context.h"

#include <cassert>

using concurrencpp::tests::wait_context;

void wait_context::wait() {
    details::atomic_wait(m_ready, uint32_t(0), std::memory_order_relaxed);
    assert(m_ready.load(std::memory_order_relaxed));
}

bool wait_context::wait_for(size_t milliseconds) {
    const auto res =
        details::atomic_wait_for(m_ready, uint32_t(0), std::chrono::milliseconds(milliseconds), std::memory_order_relaxed);
    return res == details::atomic_wait_status::ok;
}

void wait_context::notify() {
    m_ready.store(1, std::memory_order_relaxed);
    details::atomic_notify_all(m_ready);
}