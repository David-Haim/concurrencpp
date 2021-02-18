#ifndef CONCURRENCPP_THREAD_EXECUTOR_H
#define CONCURRENCPP_THREAD_EXECUTOR_H

#include "concurrencpp/threads/thread.h"
#include "concurrencpp/executors/derivable_executor.h"

#include <list>
#include <span>
#include <mutex>
#include <condition_variable>

namespace concurrencpp {
    class alignas(64) thread_executor final : public derivable_executor<thread_executor> {

       private:
        std::mutex m_lock;
        std::list<details::thread> m_workers;
        std::condition_variable m_condition;
        std::list<details::thread> m_last_retired;
        bool m_abort;
        std::atomic_bool m_atomic_abort;

        void enqueue_impl(std::unique_lock<std::mutex>& lock, task& task);
        void retire_worker(std::list<details::thread>::iterator it);

       public:
        thread_executor();
        ~thread_executor() noexcept;

        void enqueue(task task) override;
        void enqueue(std::span<task> tasks) override;

        int max_concurrency_level() const noexcept override;

        bool shutdown_requested() const noexcept override;
        void shutdown() noexcept override;
    };
}  // namespace concurrencpp

#endif
