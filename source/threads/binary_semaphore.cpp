#include "concurrencpp/threads/binary_semaphore.h"

#if defined(CRCPP_MAC_OS) && defined(CRCPP_LIBCPP_LIB)

#    include <cassert>

using concurrencpp::details::binary_semaphore;

binary_semaphore::binary_semaphore(std::ptrdiff_t desired) : m_is_signaled(desired != 0) {}

void binary_semaphore::release(std::ptrdiff_t update) {
    auto was_signaled = false;

    {
        std::unique_lock<std::mutex> lock(m_lock);
        was_signaled = m_is_signaled;
        m_is_signaled = true;
    }

    if (!was_signaled) {
        m_condition.notify_one();
    }
}

void binary_semaphore::acquire() {
    std::unique_lock<std::mutex> lock(m_lock);
    m_condition.wait(lock, [this] {
        return m_is_signaled;
    });

    assert(m_is_signaled);
    m_is_signaled = false;
}

bool binary_semaphore::try_acquire() noexcept {
    std::unique_lock<std::mutex> lock(m_lock);
    if (m_is_signaled) {
        m_is_signaled = false;
        return true;
    }

    return false;
}

bool binary_semaphore::try_acquire_until_impl(const std::chrono::time_point<std::chrono::system_clock>& abs_time) {
    std::unique_lock<std::mutex> lock(m_lock);
    m_condition.wait_until(lock, abs_time, [this] {
        return m_is_signaled;
    });

    if (m_is_signaled) {
        m_is_signaled = false;
        return true;
    }

    return false;
}

#endif
