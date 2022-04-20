#ifndef CONCURRENCPP_WORKER_THREAD_EXECUTOR_H
#define CONCURRENCPP_WORKER_THREAD_EXECUTOR_H

#include "concurrencpp/threads/thread.h"
#include "concurrencpp/threads/cache_line.h"
#include "concurrencpp/threads/binary_semaphore.h"
#include "concurrencpp/executors/derivable_executor.h"

#include <deque>
#include <mutex>

namespace concurrencpp {
    class alignas(CRCPP_CACHE_LINE_ALIGNMENT) worker_thread_executor final : public derivable_executor<worker_thread_executor> {

       private:
        std::deque<task> m_private_queue;
        std::atomic_bool m_private_atomic_abort;
        details::thread m_thread;
        alignas(CRCPP_CACHE_LINE_ALIGNMENT) std::mutex m_lock;
        std::deque<task> m_public_queue;
        details::binary_semaphore m_semaphore;
        std::atomic_bool m_atomic_abort;
        bool m_abort;

        bool drain_queue_impl();
        bool drain_queue();
        void wait_for_task(std::unique_lock<std::mutex>& lock);
        void work_loop();

        void enqueue_local(concurrencpp::task& task);
        void enqueue_local(std::span<concurrencpp::task> task);

        void enqueue_foreign(concurrencpp::task& task);
        void enqueue_foreign(std::span<concurrencpp::task> task);

       public:
        worker_thread_executor();

        void enqueue(concurrencpp::task task) override;
        void enqueue(std::span<concurrencpp::task> tasks) override;

        int max_concurrency_level() const noexcept override;

        bool shutdown_requested() const override;
        void shutdown() override;
    };
}  // namespace concurrencpp

#endif
