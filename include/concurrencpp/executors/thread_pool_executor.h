#ifndef CONCURRENCPP_THREAD_POOL_EXECUTOR_H
#define CONCURRENCPP_THREAD_POOL_EXECUTOR_H

#include "concurrencpp/threads/thread.h"
#include "concurrencpp/executors/derivable_executor.h"

#include <deque>
#include <mutex>
#include <semaphore>

namespace concurrencpp::details {
    class idle_worker_set {

        enum class status { active, idle };

        struct alignas(64) padded_flag {
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
    class alignas(64) thread_pool_worker {

       private:
        std::deque<task> m_private_queue;
        std::vector<size_t> m_idle_worker_list;
        std::atomic_bool m_atomic_abort;
        thread_pool_executor& m_parent_pool;
        const size_t m_index;
        const size_t m_pool_size;
        const std::chrono::milliseconds m_max_idle_time;
        const std::string m_worker_name;
        alignas(64) std::mutex m_lock;
        std::deque<task> m_public_queue;
        std::binary_semaphore m_semaphore;
        bool m_idle;
        bool m_abort;
        std::atomic_bool m_event_found;
        thread m_thread;

        void balance_work();

        bool wait_for_task(std::unique_lock<std::mutex>& lock) noexcept;
        bool drain_queue_impl();
        bool drain_queue();

        void work_loop() noexcept;

        void ensure_worker_active(bool first_enqueuer, std::unique_lock<std::mutex>& lock);

       public:
        thread_pool_worker(thread_pool_executor& parent_pool, size_t index, size_t pool_size, std::chrono::milliseconds max_idle_time);

        thread_pool_worker(thread_pool_worker&& rhs) noexcept;
        ~thread_pool_worker() noexcept;

        void enqueue_foreign(concurrencpp::task& task);
        void enqueue_foreign(std::span<concurrencpp::task> tasks);
        void enqueue_foreign(std::deque<concurrencpp::task>::iterator begin, std::deque<concurrencpp::task>::iterator end);
        void enqueue_foreign(std::span<concurrencpp::task>::iterator begin, std::span<concurrencpp::task>::iterator end);

        void enqueue_local(concurrencpp::task& task);
        void enqueue_local(std::span<concurrencpp::task> tasks);

        void shutdown() noexcept;

        std::chrono::milliseconds max_worker_idle_time() const noexcept;

        bool appears_empty() const noexcept;
    };
}  // namespace concurrencpp::details

namespace concurrencpp {
    class alignas(64) thread_pool_executor final : public derivable_executor<thread_pool_executor> {

        friend class details::thread_pool_worker;

       private:
        std::vector<details::thread_pool_worker> m_workers;
        alignas(64) std::atomic_size_t m_round_robin_cursor;
        alignas(64) details::idle_worker_set m_idle_workers;
        alignas(64) std::atomic_bool m_abort;

        void mark_worker_idle(size_t index) noexcept;
        void mark_worker_active(size_t index) noexcept;

        void find_idle_workers(size_t caller_index, std::vector<size_t>& buffer, size_t max_count) noexcept;

        details::thread_pool_worker& worker_at(size_t index) noexcept;

       public:
        thread_pool_executor(std::string_view pool_name, size_t pool_size, std::chrono::milliseconds max_idle_time);

        void enqueue(task task) override;
        void enqueue(std::span<task> tasks) override;

        int max_concurrency_level() const noexcept override;

        bool shutdown_requested() const noexcept override;
        void shutdown() noexcept override;

        std::chrono::milliseconds max_worker_idle_time() const noexcept;
    };
}  // namespace concurrencpp

#endif
