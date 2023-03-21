#ifndef CONCURRENCPP_ASYNC_LOCK_H
#define CONCURRENCPP_ASYNC_LOCK_H

#include "concurrencpp/utils/slist.h"
#include "concurrencpp/platform_defs.h"
#include "concurrencpp/executors/executor.h"
#include "concurrencpp/results/lazy_result.h"
#include "concurrencpp/forward_declarations.h"

namespace concurrencpp::details {
    class async_lock_awaiter {

        friend class concurrencpp::async_lock;

       private:
        async_lock& m_parent;
        std::unique_lock<std::mutex> m_lock;
        coroutine_handle<void> m_resume_handle;

       public:
        async_lock_awaiter* next = nullptr;

       public:
        async_lock_awaiter(async_lock& parent, std::unique_lock<std::mutex>& lock) noexcept;

        constexpr bool await_ready() const noexcept {
            return false;
        }

        void await_suspend(coroutine_handle<void> handle);

        constexpr void await_resume() const noexcept {}

        void retry() noexcept;
    };
}  // namespace concurrencpp::details

namespace concurrencpp {
    class scoped_async_lock;

    class CRCPP_API async_lock {

        friend class scoped_async_lock;
        friend class details::async_lock_awaiter;

       private:
        std::mutex m_awaiter_lock;
        details::slist<details::async_lock_awaiter> m_awaiters;
        bool m_locked = false;

#ifdef CRCPP_DEBUG_MODE
        std::atomic_intptr_t m_thread_count_in_critical_section {0};
#endif

        lazy_result<scoped_async_lock> lock_impl(std::shared_ptr<executor> resume_executor, bool with_raii_guard);

       public:
        ~async_lock() noexcept;

        lazy_result<scoped_async_lock> lock(std::shared_ptr<executor> resume_executor);
        lazy_result<bool> try_lock();
        void unlock();
    };

    class CRCPP_API scoped_async_lock {

       private:
        async_lock* m_lock = nullptr;
        bool m_owns = false;

       public:
        scoped_async_lock() noexcept = default;
        scoped_async_lock(scoped_async_lock&& rhs) noexcept;

        scoped_async_lock(async_lock& lock, std::defer_lock_t) noexcept;
        scoped_async_lock(async_lock& lock, std::adopt_lock_t) noexcept;

        ~scoped_async_lock() noexcept;

        lazy_result<void> lock(std::shared_ptr<executor> resume_executor);
        lazy_result<bool> try_lock();
        void unlock();

        bool owns_lock() const noexcept;
        explicit operator bool() const noexcept;

        void swap(scoped_async_lock& rhs) noexcept;
        async_lock* release() noexcept;
        async_lock* mutex() const noexcept;
    };
}  // namespace concurrencpp

#endif
