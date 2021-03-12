#ifndef CONCURRENCPP_TIMER_QUEUE_H
#define CONCURRENCPP_TIMER_QUEUE_H

#include "constants.h"
#include "timer.h"

#include "concurrencpp/errors.h"

#include "concurrencpp/utils/bind.h"
#include "concurrencpp/threads/thread.h"

#include <mutex>
#include <memory>
#include <chrono>
#include <vector>
#include <condition_variable>

#include <cassert>

namespace concurrencpp::details {
    enum class timer_request { add, remove };
}

namespace concurrencpp {
    class timer_queue : public std::enable_shared_from_this<timer_queue> {

       public:
        using timer_ptr = std::shared_ptr<details::timer_state_base>;
        using clock_type = std::chrono::high_resolution_clock;
        using time_point = std::chrono::time_point<std::chrono::high_resolution_clock>;
        using request_queue = std::vector<std::pair<timer_ptr, details::timer_request>>;

        friend class concurrencpp::timer;

       private:
        std::atomic_bool m_atomic_abort;
        std::mutex m_lock;
        request_queue m_request_queue;
        details::thread m_worker;
        std::condition_variable m_condition;
        bool m_abort;
        bool m_idle;
        const std::chrono::milliseconds m_max_waiting_time;

        details::thread ensure_worker_thread(std::unique_lock<std::mutex>& lock);

        void add_timer(std::unique_lock<std::mutex>& lock, timer_ptr new_timer);
        void remove_timer(timer_ptr existing_timer);

        template<class callable_type>
        timer_ptr make_timer_impl(size_t due_time,
                                  size_t frequency,
                                  std::shared_ptr<concurrencpp::executor> executor,
                                  bool is_oneshot,
                                  callable_type&& callable) {
            assert(static_cast<bool>(executor));

            using decayed_type = typename std::decay_t<callable_type>;

            auto timer_state = std::make_shared<details::timer_state<decayed_type>>(due_time,
                                                                                    frequency,
                                                                                    std::move(executor),
                                                                                    weak_from_this(),
                                                                                    is_oneshot,
                                                                                    std::forward<callable_type>(callable));

            std::unique_lock<std::mutex> lock(m_lock);
            if (m_abort) {
                throw errors::runtime_shutdown(details::consts::k_timer_queue_shutdown_err_msg);
            }

            auto old_thread = ensure_worker_thread(lock);
            add_timer(lock, timer_state);

            if (old_thread.joinable()) {
                old_thread.join();
            }

            return timer_state;
        }

        void work_loop() noexcept;

       public:
        timer_queue(std::chrono::milliseconds max_waiting_time) noexcept;
        ~timer_queue() noexcept;

        void shutdown() noexcept;
        bool shutdown_requested() const noexcept;

        template<class callable_type, class... argumet_types>
        timer make_timer(std::chrono::milliseconds due_time,
                         std::chrono::milliseconds frequency,
                         std::shared_ptr<concurrencpp::executor> executor,
                         callable_type&& callable,
                         argumet_types&&... arguments) {
            if (!static_cast<bool>(executor)) {
                throw std::invalid_argument(details::consts::k_timer_queue_make_timer_executor_null_err_msg);
            }

            return make_timer_impl(due_time.count(),
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
            if (!static_cast<bool>(executor)) {
                throw std::invalid_argument(details::consts::k_timer_queue_make_oneshot_timer_executor_null_err_msg);
            }

            return make_timer_impl(due_time.count(),
                                   0,
                                   std::move(executor),
                                   true,
                                   details::bind(std::forward<callable_type>(callable), std::forward<argumet_types>(arguments)...));
        }

        result<void> make_delay_object(std::chrono::milliseconds due_time, std::shared_ptr<concurrencpp::executor> executor);

        std::chrono::milliseconds max_worker_idle_time() const noexcept;
    };
}  // namespace concurrencpp

#endif  // CONCURRENCPP_TIMER_QUEUE_H
