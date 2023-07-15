#ifndef CONCURRENCPP_WORKER_THREAD_EXECUTOR_H
#define CONCURRENCPP_WORKER_THREAD_EXECUTOR_H

#include "concurrencpp/threads/thread.h"
#include "concurrencpp/threads/cache_line.h"
#include "concurrencpp/executors/derivable_executor.h"

#include <deque>
#include <mutex>
#include <semaphore>

namespace concurrencpp {
    class CRCPP_API alignas(CRCPP_CACHE_LINE_ALIGNMENT) worker_thread_executor final :
        public derivable_executor<worker_thread_executor> {

       private:
        std::deque<task> m_private_queue;
        std::atomic_bool m_private_atomic_abort;
        alignas(CRCPP_CACHE_LINE_ALIGNMENT) std::mutex m_lock;
        std::deque<task> m_public_queue;
        std::binary_semaphore m_semaphore;
        details::thread m_thread;
        std::atomic_bool m_atomic_abort;
        bool m_abort;
        const std::function<void(std::string_view)> m_thread_started_callback;
        const std::function<void(std::string_view)> m_thread_terminated_callback;

        void make_os_worker_thread();
        bool drain_queue_impl();
        bool drain_queue();
        void wait_for_task(std::unique_lock<std::mutex>& lock);
        void work_loop();

        void enqueue_local(concurrencpp::task& task);
        void enqueue_local(std::span<concurrencpp::task> task);

        void enqueue_foreign(concurrencpp::task& task);
        void enqueue_foreign(std::span<concurrencpp::task> task);

       public:
        worker_thread_executor(const std::function<void(std::string_view thread_name)>& thread_started_callback = {},
                               const std::function<void(std::string_view thread_name)>& thread_terminated_callback = {});

        void enqueue(concurrencpp::task task) override;
        void enqueue(std::span<concurrencpp::task> tasks) override;

        int max_concurrency_level() const noexcept override;

        bool shutdown_requested() const override;
        void shutdown() override;
    };
}  // namespace concurrencpp

#endif
