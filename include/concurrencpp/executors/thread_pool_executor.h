#ifndef CONCURRENCPP_THREAD_POOL_EXECUTOR_H
#define CONCURRENCPP_THREAD_POOL_EXECUTOR_H

#include "concurrencpp/threads/thread.h"
#include "concurrencpp/threads/cache_line.h"
#include "concurrencpp/executors/derivable_executor.h"

#include <deque>
#include <mutex>

namespace concurrencpp::details {
    class idle_worker_set {

        enum class status { active, idle };

        struct alignas(CRCPP_CACHE_LINE_ALIGNMENT) padded_flag {
            std::atomic<status> flag {status::active};
        };

       private:
        std::atomic_intptr_t m_approx_size;
        const std::unique_ptr<padded_flag[]> m_idle_flags;
        const size_t m_size;

        bool try_acquire_flag(size_t index) noexcept;

       public:
        idle_worker_set(size_t size);

        void set_idle(size_t idle_thread) noexcept;
        void set_active(size_t idle_thread) noexcept;

        size_t find_idle_worker(size_t caller_index) noexcept;
        void find_idle_workers(size_t caller_index, std::vector<size_t>& result_buffer, size_t max_count) noexcept;
    };
}  // namespace concurrencpp::details

namespace concurrencpp::details {
    class thread_pool_worker;
}  // namespace concurrencpp::details

namespace concurrencpp {
    class CRCPP_API alignas(CRCPP_CACHE_LINE_ALIGNMENT) thread_pool_executor final : public derivable_executor<thread_pool_executor> {

        friend class details::thread_pool_worker;

       private:
        std::vector<details::thread_pool_worker> m_workers;
        alignas(CRCPP_CACHE_LINE_ALIGNMENT) std::atomic_size_t m_round_robin_cursor;
        alignas(CRCPP_CACHE_LINE_ALIGNMENT) details::idle_worker_set m_idle_workers;
        alignas(CRCPP_CACHE_LINE_ALIGNMENT) std::atomic_bool m_abort;

        void mark_worker_idle(size_t index) noexcept;
        void mark_worker_active(size_t index) noexcept;
        void find_idle_workers(size_t caller_index, std::vector<size_t>& buffer, size_t max_count) noexcept;

        details::thread_pool_worker& worker_at(size_t index) noexcept;

       public:
        thread_pool_executor(std::string_view pool_name,
                             size_t pool_size,
                             std::chrono::milliseconds max_idle_time,
                             const std::function<void(std::string_view thread_name)>& thread_started_callback = {},
                             const std::function<void(std::string_view thread_name)>& thread_terminated_callback = {});

        ~thread_pool_executor() override;

        void enqueue(task task) override;
        void enqueue(std::span<task> tasks) override;

        int max_concurrency_level() const noexcept override;

        bool shutdown_requested() const override;
        void shutdown() override;

        std::chrono::milliseconds max_worker_idle_time() const noexcept;
    };
}  // namespace concurrencpp

#endif
