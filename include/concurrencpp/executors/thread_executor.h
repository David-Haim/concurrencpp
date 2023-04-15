#ifndef CONCURRENCPP_THREAD_EXECUTOR_H
#define CONCURRENCPP_THREAD_EXECUTOR_H

#include "concurrencpp/threads/thread.h"
#include "concurrencpp/threads/cache_line.h"
#include "concurrencpp/executors/executor.h"

#include <list>
#include <mutex>
#include <condition_variable>

namespace concurrencpp {
    class CRCPP_API alignas(CRCPP_CACHE_LINE_ALIGNMENT) thread_executor final : public executor {

       private:
        std::mutex m_lock;
        std::list<details::thread> m_workers;
        std::condition_variable m_condition;
        std::list<details::thread> m_last_retired;
        bool m_abort;
        std::atomic_bool m_atomic_abort;

        void retire_worker(std::list<details::thread>::iterator it);

        template<class functor_type>
        void enqueue_impl(functor_type&& functor) {
            std::unique_lock<std::mutex> lock(m_lock);
            if (m_abort) {
                throw_runtime_shutdown_exception(name);
            }

            auto& new_thread = m_workers.emplace_front();
            new_thread = details::thread(make_executor_worker_name(name),
                                         [this, self_it = m_workers.begin(), func = std::move(functor)]() mutable {
                                             func();
                                             retire_worker(self_it);
                                         });
        }

       public:
        thread_executor();
        ~thread_executor() noexcept;

        void enqueue(task& task) override;

        int max_concurrency_level() const noexcept override;

        bool shutdown_requested() const override;
        void shutdown() override;

        template<class callable_type, class... argument_types>
        null_result post(callable_type&& callable, argument_types&&... arguments) {
            enqueue_impl(details::bind_with_try_catch(std::forward<callable_type>(callable), std::forward<argument_types>(arguments)...));
            return {};
        }
    };
}  // namespace concurrencpp

#endif
