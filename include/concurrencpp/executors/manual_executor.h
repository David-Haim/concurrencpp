#ifndef CONCURRENCPP_MANUAL_EXECUTOR_H
#define CONCURRENCPP_MANUAL_EXECUTOR_H

#include "concurrencpp/executors/derivable_executor.h"

#include <deque>

namespace concurrencpp {
    class alignas(64) manual_executor final : public derivable_executor<manual_executor> {

       private:
        mutable std::mutex m_lock;
        std::deque<task> m_tasks;
        std::condition_variable m_condition;
        bool m_abort;
        std::atomic_bool m_atomic_abort;

       public:
        manual_executor();

        void enqueue(task task) override;
        void enqueue(std::span<task> tasks) override;

        int max_concurrency_level() const noexcept override;

        void shutdown() noexcept override;
        bool shutdown_requested() const noexcept override;

        size_t size() const noexcept;
        bool empty() const noexcept;

        bool loop_once();
        bool loop_once(std::chrono::milliseconds max_waiting_time);

        size_t loop(size_t max_count);

        size_t clear() noexcept;

        void wait_for_task();
        bool wait_for_task(std::chrono::milliseconds max_waiting_time);
    };
}  // namespace concurrencpp

#endif
