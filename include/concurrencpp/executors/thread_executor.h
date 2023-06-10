#ifndef CONCURRENCPP_THREAD_EXECUTOR_H
#define CONCURRENCPP_THREAD_EXECUTOR_H

#include "concurrencpp/threads/thread.h"
#include "concurrencpp/threads/cache_line.h"
#include "concurrencpp/executors/derivable_executor.h"

#include <list>
#include <span>
#include <mutex>
#include <condition_variable>

namespace concurrencpp {
    class CRCPP_API alignas(CRCPP_CACHE_LINE_ALIGNMENT) thread_executor final : public derivable_executor<thread_executor> {

       private:
        std::mutex m_lock;
        std::list<details::thread> m_workers;
        std::condition_variable m_condition;
        std::list<details::thread> m_last_retired;
        bool m_abort;
        std::atomic_bool m_atomic_abort;
        const std::function<void(std::string_view thread_name)> m_thread_started_callback;
        const std::function<void(std::string_view thread_name)> m_thread_terminated_callback;

        void enqueue_impl(std::unique_lock<std::mutex>& lock, task& task);
        void retire_worker(std::list<details::thread>::iterator it);

       public:
        thread_executor(const std::function<void(std::string_view thread_name)>& thread_started_callback = {},
                        const std::function<void(std::string_view thread_name)>& thread_terminated_callback = {});
        ~thread_executor() noexcept;

        void enqueue(task task) override;
        void enqueue(std::span<task> tasks) override;

        int max_concurrency_level() const noexcept override;

        bool shutdown_requested() const override;
        void shutdown() override;
    };
}  // namespace concurrencpp

#endif
