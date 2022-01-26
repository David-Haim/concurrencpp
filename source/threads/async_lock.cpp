#include "concurrencpp/results/resume_on.h"
#include "concurrencpp/threads/constants.h"
#include "concurrencpp/threads/async_lock.h"
#include "concurrencpp/executors/executor.h"

namespace concurrencpp::details {
    class async_lock_awaiter {

        friend class concurrencpp::async_lock;

       private:
        async_lock& m_parent;
        async_lock_awaiter* m_next = nullptr;
        coroutine_handle<void> m_resume_handle;
        bool m_locked = false;

       public:
        async_lock_awaiter(async_lock& parent) noexcept : m_parent(parent) {}

        static bool await_ready() noexcept {
            return false;
        }

        bool await_suspend(coroutine_handle<void> handle) {
            assert(m_locked == false);
            assert(static_cast<bool>(handle));
            assert(!handle.done());
            assert(!static_cast<bool>(m_resume_handle));

            m_resume_handle = handle;

            std::unique_lock<std::mutex> lock(m_parent.m_awaiter_lock);
            if (!m_parent.m_locked) {
                m_parent.m_locked = true;

#ifdef CRCPP_DEBUG_MODE
                assert(!static_cast<bool>(m_parent.m_owning_coro));
                m_parent.m_owning_coro = handle;
#endif

                lock.unlock();
                m_locked = true;
                return false;
            }

#ifdef CRCPP_DEBUG_MODE
            assert(static_cast<bool>(m_parent.m_owning_coro));
#endif

            m_parent.enqueue_awaiter(lock, *this);
            lock.unlock();
            return true;
        }

        bool await_resume() const noexcept {
            return m_locked;
        }

        void retry() noexcept {
            m_resume_handle.resume();
        }
    };

    class async_try_lock_awaiter {

        friend class concurrencpp::async_lock;

       private:
        async_lock& m_parent;
        bool m_locked = false;

        async_try_lock_awaiter(async_lock& parent) noexcept : m_parent(parent) {}

        static bool await_ready() noexcept {
            return false;
        }

        bool await_suspend(coroutine_handle<void> handle) {
            assert(m_locked == false);
            assert(static_cast<bool>(handle));
            assert(!handle.done());

            std::unique_lock<std::mutex> lock(m_parent.m_awaiter_lock);
            if (!m_parent.m_locked) {
                m_parent.m_locked = true;

#ifdef CRCPP_DEBUG_MODE
                assert(!static_cast<bool>(m_parent.m_owning_coro));
                m_parent.m_owning_coro = handle;
#endif

                lock.unlock();
                m_locked = true;
                return false;
            }

#ifdef CRCPP_DEBUG_MODE
            assert(static_cast<bool>(m_parent.m_owning_coro));
#endif

            lock.unlock();
            return false;
        }

        bool await_resume() const noexcept {
            return m_locked;
        }
    };
}  // namespace concurrencpp::details

using concurrencpp::async_lock;
using concurrencpp::scoped_async_lock;
using concurrencpp::details::async_lock_awaiter;

async_lock::~async_lock() noexcept {
#ifdef CRCPP_DEBUG_MODE
    std::unique_lock<std::mutex> lock(m_awaiter_lock);
    assert(!m_locked && "async_lock is dstroyed while it's locked.");
#endif
}

void async_lock::enqueue_awaiter(std::unique_lock<std::mutex>& lock, details::async_lock_awaiter& awaiter_node) noexcept {
    assert(lock.owns_lock());

    if (m_head == nullptr) {
        assert(m_tail == nullptr);
        m_head = m_tail = &awaiter_node;
        return;
    }

    m_tail->m_next = &awaiter_node;
    m_tail = &awaiter_node;
}

async_lock_awaiter* async_lock::try_dequeue_awaiter(std::unique_lock<std::mutex>& lock) noexcept {
    assert(lock.owns_lock());

    const auto node = m_head;
    if (node == nullptr) {
        return nullptr;
    }

    m_head = m_head->m_next;
    if (m_head == nullptr) {
        m_tail = nullptr;
    }

    return node;
}

concurrencpp::lazy_result<scoped_async_lock> async_lock::lock_impl(std::shared_ptr<executor> resume_executor) {
    auto resume_synchronously = true;

    while (true) {
        async_lock_awaiter awaiter(*this);
        const auto locked = co_await awaiter;
        if (locked) {
            break;
        }

        resume_synchronously = false;
    }

#ifdef CRCPP_DEBUG_MODE
    const auto current_count = m_thread_count_in_critical_section.fetch_add(1, std::memory_order_relaxed);
    assert(current_count == 0);
#endif

    if (!resume_synchronously) {
        co_await resume_on(resume_executor);
    }

    co_return scoped_async_lock(*this, std::adopt_lock);
}

concurrencpp::lazy_result<scoped_async_lock> async_lock::lock(std::shared_ptr<executor> resume_executor) {
    if (!static_cast<bool>(resume_executor)) {
        throw std::invalid_argument(details::consts::k_async_lock_null_resume_executor_err_msg);
    }

    return lock_impl(std::move(resume_executor));
}

concurrencpp::lazy_result<bool> async_lock::try_lock() noexcept {
    details::async_try_lock_awaiter awaiter(*this);
    const auto res = co_await awaiter;

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

    assert(static_cast<bool>(m_owning_coro));
    m_owning_coro = {};
#endif

    const auto awaiter = try_dequeue_awaiter(lock);
    lock.unlock();

    if (awaiter != nullptr) {
        awaiter->retry();
    }
}

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
    if (m_lock == nullptr) {
        throw std::system_error(static_cast<int>(std::errc::operation_not_permitted), std::system_category());
    } else if (m_owns) {
        throw std::system_error(static_cast<int>(std::errc::resource_deadlock_would_occur), std::system_category());
    } else {
        co_await m_lock->lock(resume_executor);
        m_owns = true;
    }
}

concurrencpp::lazy_result<bool> scoped_async_lock::try_lock() {
    if (m_lock == nullptr) {
        throw std::system_error(static_cast<int>(std::errc::operation_not_permitted), std::system_category());
    } else if (m_owns) {
        throw std::system_error(static_cast<int>(std::errc::resource_deadlock_would_occur), std::system_category());
    } else {
        m_owns = co_await m_lock->try_lock();
    }

    co_return m_owns;
}

void scoped_async_lock::unlock() {
    if (!m_owns) {
        throw std::system_error(static_cast<int>(std::errc::operation_not_permitted), std::system_category());
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
