#include "concurrencpp/results/resume_on.h"
#include "concurrencpp/threads/constants.h"
#include "concurrencpp/threads/async_lock.h"
#include "concurrencpp/executors/executor.h"

using concurrencpp::async_lock;
using concurrencpp::scoped_async_lock;
using concurrencpp::details::async_lock_awaiter;

/*
    async_lock_awaiter
*/

async_lock_awaiter::async_lock_awaiter(async_lock& parent, std::unique_lock<std::mutex>& lock) noexcept :
    m_parent(parent), m_lock(std::move(lock)) {}

void async_lock_awaiter::await_suspend(coroutine_handle<void> handle) {
    assert(static_cast<bool>(handle));
    assert(!handle.done());
    assert(!static_cast<bool>(m_resume_handle));
    assert(m_lock.owns_lock());

    m_resume_handle = handle;
    m_parent.m_awaiters.push_back(*this);

    auto lock = std::move(m_lock);  // will unlock underlying lock
}

void async_lock_awaiter::retry() noexcept {
    m_resume_handle.resume();
}

/*
    async_lock
*/

async_lock::~async_lock() noexcept {
#ifdef CRCPP_DEBUG_MODE
    std::unique_lock<std::mutex> lock(m_awaiter_lock);
    assert(!m_locked && "async_lock is dstroyed while it's locked.");
#endif
}

concurrencpp::lazy_result<scoped_async_lock> async_lock::lock_impl(std::shared_ptr<executor> resume_executor, bool with_raii_guard) {
    auto resume_synchronously = true;  // indicates if the locking coroutine managed to lock the lock on first attempt

    while (true) {
        std::unique_lock<std::mutex> lock(m_awaiter_lock);
        if (!m_locked) {
            m_locked = true;
            lock.unlock();
            break;
        }

        co_await async_lock_awaiter(*this, lock);

        resume_synchronously =
            false;  // if we haven't managed to lock the lock on first attempt, we need to resume using resume_executor
    }

    if (!resume_synchronously) {
        try {
            co_await resume_on(resume_executor);
        } catch (...) {
            std::unique_lock<std::mutex> lock(m_awaiter_lock);
            assert(m_locked);
            m_locked = false;
            const auto awaiter = m_awaiters.pop_front();
            lock.unlock();

            if (awaiter != nullptr) {
                awaiter->retry();
            }

            throw;
        }
    }

#ifdef CRCPP_DEBUG_MODE
    const auto current_count = m_thread_count_in_critical_section.fetch_add(1, std::memory_order_relaxed);
    assert(current_count == 0);
#endif

    if (with_raii_guard) {
        co_return scoped_async_lock(*this, std::adopt_lock);
    }

    co_return scoped_async_lock(*this, std::defer_lock);
}

concurrencpp::lazy_result<scoped_async_lock> async_lock::lock(std::shared_ptr<executor> resume_executor) {
    if (!static_cast<bool>(resume_executor)) {
        throw std::invalid_argument(details::consts::k_async_lock_null_resume_executor_err_msg);
    }

    return lock_impl(std::move(resume_executor), true);
}

concurrencpp::lazy_result<bool> async_lock::try_lock() {
    auto res = false;

    std::unique_lock<std::mutex> lock(m_awaiter_lock);
    if (!m_locked) {
        m_locked = true;
        lock.unlock();
        res = true;
    } else {
        lock.unlock();
    }

#ifdef CRCPP_DEBUG_MODE
    if (res) {
        const auto current_count = m_thread_count_in_critical_section.fetch_add(1, std::memory_order_relaxed);
        assert(current_count == 0);
    }
#endif

    co_return res;
}

void async_lock::unlock() {
    std::unique_lock<std::mutex> lock(m_awaiter_lock);
    if (!m_locked) {  // trying to unlocked non-owned mutex
        lock.unlock();
        throw std::system_error(static_cast<int>(std::errc::operation_not_permitted),
                                std::system_category(),
                                details::consts::k_async_lock_unlock_invalid_lock_err_msg);
    }

    m_locked = false;

#ifdef CRCPP_DEBUG_MODE
    const auto current_count = m_thread_count_in_critical_section.fetch_sub(1, std::memory_order_relaxed);
    assert(current_count == 1);
#endif

    const auto awaiter = m_awaiters.pop_front();
    lock.unlock();

    if (awaiter != nullptr) {
        awaiter->retry();
    }
}

/*
 *  scoped_async_lock
 */

scoped_async_lock::scoped_async_lock(scoped_async_lock&& rhs) noexcept :
    m_lock(std::exchange(rhs.m_lock, nullptr)), m_owns(std::exchange(rhs.m_owns, false)) {}

scoped_async_lock::scoped_async_lock(async_lock& lock, std::defer_lock_t) noexcept : m_lock(&lock), m_owns(false) {}

scoped_async_lock::scoped_async_lock(async_lock& lock, std::adopt_lock_t) noexcept : m_lock(&lock), m_owns(true) {}

scoped_async_lock::~scoped_async_lock() noexcept {
    if (m_owns && m_lock != nullptr) {
        m_lock->unlock();
    }
}

concurrencpp::lazy_result<void> scoped_async_lock::lock(std::shared_ptr<executor> resume_executor) {
    if (!static_cast<bool>(resume_executor)) {
        throw std::invalid_argument(details::consts::k_scoped_async_lock_null_resume_executor_err_msg);
    }

    if (m_lock == nullptr) {
        throw std::system_error(static_cast<int>(std::errc::operation_not_permitted),
                                std::system_category(),
                                details::consts::k_scoped_async_lock_lock_no_mutex_err_msg);
    } else if (m_owns) {
        throw std::system_error(static_cast<int>(std::errc::resource_deadlock_would_occur),
                                std::system_category(),
                                details::consts::k_scoped_async_lock_lock_deadlock_err_msg);
    } else {
        co_await m_lock->lock_impl(std::move(resume_executor), false);
        m_owns = true;
    }
}

concurrencpp::lazy_result<bool> scoped_async_lock::try_lock() {
    if (m_lock == nullptr) {
        throw std::system_error(static_cast<int>(std::errc::operation_not_permitted),
                                std::system_category(),
                                concurrencpp::details::consts::k_scoped_async_lock_try_lock_no_mutex_err_msg);
    } else if (m_owns) {
        throw std::system_error(static_cast<int>(std::errc::resource_deadlock_would_occur),
                                std::system_category(),
                                concurrencpp::details::consts::k_scoped_async_lock_try_lock_deadlock_err_msg);
    } else {
        m_owns = co_await m_lock->try_lock();
    }

    co_return m_owns;
}

void scoped_async_lock::unlock() {
    if (!m_owns) {
        throw std::system_error(static_cast<int>(std::errc::operation_not_permitted),
                                std::system_category(),
                                concurrencpp::details::consts::k_scoped_async_lock_unlock_invalid_lock_err_msg);
    } else if (m_lock != nullptr) {
        m_lock->unlock();
        m_owns = false;
    }
}

bool scoped_async_lock::owns_lock() const noexcept {
    return m_owns;
}

scoped_async_lock::operator bool() const noexcept {
    return owns_lock();
}

void scoped_async_lock::swap(scoped_async_lock& rhs) noexcept {
    std::swap(m_lock, rhs.m_lock);
    std::swap(m_owns, rhs.m_owns);
}

async_lock* scoped_async_lock::release() noexcept {
    m_owns = false;
    return std::exchange(m_lock, nullptr);
}

async_lock* scoped_async_lock::mutex() const noexcept {
    return m_lock;
}