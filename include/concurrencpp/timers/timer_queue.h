#ifndef CONCURRENCPP_TIMER_QUEUE_H
#define CONCURRENCPP_TIMER_QUEUE_H

#include "timer.h"
#include "concurrencpp/errors.h"
#include "concurrencpp/utils/bind.h"
#include "concurrencpp/threads/thread.h"
#include "concurrencpp/results/lazy_result.h"

#include <mutex>
#include <memory>
#include <chrono>
#include <vector>
#include <condition_variable>

#include <cassert>

namespace concurrencpp {
    class CRCPP_API timer_queue : public std::enable_shared_from_this<timer_queue> {

       public:
        using timer_ptr = std::shared_ptr<details::timer_state_base>;
        using clock_type = std::chrono::high_resolution_clock;
        using time_point = std::chrono::time_point<std::chrono::high_resolution_clock>;
  
        friend class concurrencpp::timer;

       private:
        std::atomic_bool m_atomic_abort;
        std::mutex m_lock;
        std::vector<timer_ptr> m_add_timer_queue;
        std::vector<timer_ptr> m_remove_timer_queue;
        details::thread m_worker;
        std::condition_variable m_condition;
        bool m_abort;
        bool m_idle;
        const std::chrono::milliseconds m_max_waiting_time;
        const std::function<void(std::string_view thread_name)> m_thread_started_callback;
        const std::function<void(std::string_view thread_name)> m_thread_terminated_callback;

        details::thread ensure_worker_thread(std::unique_lock<std::mutex>& lock);

        void add_internal_timer(std::unique_lock<std::mutex>& lock, timer_ptr new_timer);
        void remove_internal_timer(timer_ptr existing_timer);

        void add_timer(std::unique_lock<std::mutex>& lock, timer_ptr new_timer, const char* calling_method);

        lazy_result<void> make_delay_object_impl(std::chrono::milliseconds due_time,
                                                 std::shared_ptr<concurrencpp::timer_queue> self,
                                                 std::shared_ptr<concurrencpp::executor> executor);

        template<class callable_type>
        timer_ptr make_timer_impl(const char* calling_method,
                                  size_t due_time,
                                  size_t frequency,
                                  std::shared_ptr<concurrencpp::executor> executor,
                                  bool is_oneshot,
                                  callable_type&& callable) {
            assert(calling_method != nullptr);
            assert(static_cast<bool>(executor));

            using decayed_type = typename std::decay_t<callable_type>;

            auto timer_state = std::make_shared<details::timer_state<decayed_type>>(due_time,
                                                                                    frequency,
                                                                                    std::move(executor),
                                                                                    weak_from_this(),
                                                                                    is_oneshot,
                                                                                    std::forward<callable_type>(callable));
            {
                std::unique_lock<std::mutex> lock(m_lock);
                add_timer(lock, timer_state, calling_method);
            }

            return timer_state;
        }

        void work_loop();

       public:
        static constexpr std::string_view k_class_name = "timer_queue";

        timer_queue(std::chrono::milliseconds max_waiting_time,
                    const std::function<void(std::string_view thread_name)>& thread_started_callback = {},
                    const std::function<void(std::string_view thread_name)>& thread_terminated_callback = {});
        ~timer_queue() noexcept;

        void shutdown();
        bool shutdown_requested() const noexcept;

        template<class callable_type, class... argumet_types>
        timer make_timer(std::chrono::milliseconds due_time,
                         std::chrono::milliseconds frequency,
                         std::shared_ptr<concurrencpp::executor> executor,
                         callable_type&& callable,
                         argumet_types&&... arguments) {
            details::throw_helper::throw_if_null_argument(executor, k_class_name, "make_timer", "executor");

            return make_timer_impl("make_timer",
                                   due_time.count(),
                                   frequency.count(),
                                   std::move(executor),
                                   false,
                                   details::bind(std::forward<callable_type>(callable), std::forward<argumet_types>(arguments)...));
        }

        template<class callable_type, class... argumet_types>
        timer make_one_shot_timer(std::chrono::milliseconds due_time,
                                  std::shared_ptr<concurrencpp::executor> executor,
                                  callable_type&& callable,
                                  argumet_types&&... arguments) {
            details::throw_helper::throw_if_null_argument(executor, k_class_name, "make_one_shot_timer", "executor");

            return make_timer_impl("make_one_shot_timer",
                                   due_time.count(),
                                   0,
                                   std::move(executor),
                                   true,
                                   details::bind(std::forward<callable_type>(callable), std::forward<argumet_types>(arguments)...));
        }

        lazy_result<void> make_delay_object(std::chrono::milliseconds due_time, std::shared_ptr<concurrencpp::executor> executor);

        std::chrono::milliseconds max_worker_idle_time() const noexcept;
    };
}  // namespace concurrencpp

#endif  // CONCURRENCPP_TIMER_QUEUE_H
