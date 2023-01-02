#ifndef CONCURRENCPP_MANUAL_EXECUTOR_H
#define CONCURRENCPP_MANUAL_EXECUTOR_H

#include "concurrencpp/threads/cache_line.h"
#include "concurrencpp/executors/derivable_executor.h"

#include <deque>
#include <mutex>
#include <chrono>
#include <condition_variable>

namespace concurrencpp {
    class CRCPP_API alignas(CRCPP_CACHE_LINE_ALIGNMENT) manual_executor final : public derivable_executor<manual_executor> {

       private:
        mutable std::mutex m_lock;
        std::deque<task> m_tasks;
        std::condition_variable m_condition;
        bool m_abort;
        std::atomic_bool m_atomic_abort;

        template<class clock_type, class duration_type>
        static std::chrono::system_clock::time_point to_system_time_point(
            std::chrono::time_point<clock_type, duration_type> time_point) noexcept(noexcept(clock_type::now())) {
            const auto src_now = clock_type::now();
            const auto dst_now = std::chrono::system_clock::now();
            return dst_now + std::chrono::duration_cast<std::chrono::milliseconds>(time_point - src_now);
        }

        static std::chrono::system_clock::time_point time_point_from_now(std::chrono::milliseconds ms) noexcept {
            return std::chrono::system_clock::now() + ms;
        }

        size_t loop_impl(size_t max_count);
        size_t loop_until_impl(size_t max_count, std::chrono::time_point<std::chrono::system_clock> deadline);

        void wait_for_tasks_impl(size_t count);
        size_t wait_for_tasks_impl(size_t count, std::chrono::time_point<std::chrono::system_clock> deadline);

       public:
        manual_executor();

        void enqueue(task task) override;
        void enqueue(std::span<task> tasks) override;

        int max_concurrency_level() const noexcept override;

        void shutdown() override;
        bool shutdown_requested() const override;

        size_t size() const;
        bool empty() const;

        size_t clear();

        bool loop_once();
        bool loop_once_for(std::chrono::milliseconds max_waiting_time);

        template<class clock_type, class duration_type>
        bool loop_once_until(std::chrono::time_point<clock_type, duration_type> timeout_time) {
            return loop_until_impl(1, to_system_time_point(timeout_time));
        }

        size_t loop(size_t max_count);
        size_t loop_for(size_t max_count, std::chrono::milliseconds max_waiting_time);

        template<class clock_type, class duration_type>
        size_t loop_until(size_t max_count, std::chrono::time_point<clock_type, duration_type> timeout_time) {
            return loop_until_impl(max_count, to_system_time_point(timeout_time));
        }

        void wait_for_task();
        bool wait_for_task_for(std::chrono::milliseconds max_waiting_time);

        template<class clock_type, class duration_type>
        bool wait_for_task_until(std::chrono::time_point<clock_type, duration_type> timeout_time) {
            return wait_for_tasks_impl(1, to_system_time_point(timeout_time)) == 1;
        }

        void wait_for_tasks(size_t count);
        size_t wait_for_tasks_for(size_t count, std::chrono::milliseconds max_waiting_time);

        template<class clock_type, class duration_type>
        size_t wait_for_tasks_until(size_t count, std::chrono::time_point<clock_type, duration_type> timeout_time) {
            return wait_for_tasks_impl(count, to_system_time_point(timeout_time));
        }
    };
}  // namespace concurrencpp

#endif
