#ifndef CONCURRENCPP_WORKER_THREAD_EXECUTOR_H
#define CONCURRENCPP_WORKER_THREAD_EXECUTOR_H

#include "concurrencpp/executors/derivable_executor.h"

#include "concurrencpp/threads/thread.h"

#include <deque>

namespace concurrencpp {
    class alignas(64) worker_thread_executor final : public derivable_executor<worker_thread_executor> {

       private:
        std::deque<task> m_private_queue;
        std::atomic_bool m_private_atomic_abort;
        details::thread m_thread;
        const char m_padding[64] = {};
        std::mutex m_lock;
        std::deque<task> m_public_queue;
        std::condition_variable m_condition;
        bool m_abort;
        std::atomic_bool m_atomic_abort;

        bool drain_queue_impl();
        bool drain_queue();
        void work_loop() noexcept;

        void enqueue_local(concurrencpp::task& task);
        void enqueue_local(std::span<concurrencpp::task> task);

        void enqueue_foreign(concurrencpp::task& task);
        void enqueue_foreign(std::span<concurrencpp::task> task);

       public:
        worker_thread_executor();

        void enqueue(concurrencpp::task task) override;
        void enqueue(std::span<concurrencpp::task> tasks) override;

        int max_concurrency_level() const noexcept override;

        bool shutdown_requested() const noexcept override;
        void shutdown() noexcept override;
    };
}  // namespace concurrencpp

#endif
