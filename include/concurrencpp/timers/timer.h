#ifndef CONCURRENCPP_TIMER_H
#define CONCURRENCPP_TIMER_H

#include "concurrencpp/forward_declarations.h"
#include "concurrencpp/platform_defs.h"

#include <atomic>
#include <memory>
#include <chrono>

namespace concurrencpp::details {
    class CRCPP_API timer_state_base : public std::enable_shared_from_this<timer_state_base> {

       public:
        using clock_type = std::chrono::high_resolution_clock;
        using time_point = std::chrono::time_point<clock_type>;
        using milliseconds = std::chrono::milliseconds;

       private:
        const std::weak_ptr<timer_queue> m_timer_queue;
        const std::shared_ptr<executor> m_executor;
        const size_t m_due_time;
        std::atomic_size_t m_frequency;
        time_point m_deadline;  // set by the c.tor, changed only by the timer_queue thread.
        std::atomic_bool m_cancelled;
        const bool m_is_oneshot;

        static time_point make_deadline(milliseconds diff) noexcept {
            return clock_type::now() + diff;
        }

       public:
        timer_state_base(size_t due_time,
                         size_t frequency,
                         std::shared_ptr<concurrencpp::executor> executor,
                         std::weak_ptr<concurrencpp::timer_queue> timer_queue,
                         bool is_oneshot) noexcept;

        virtual ~timer_state_base() noexcept = default;

        virtual void execute() = 0;

        void fire();

        bool expired(const time_point now) const noexcept {
            return m_deadline <= now;
        }

        time_point get_deadline() const noexcept {
            return m_deadline;
        }

        size_t get_frequency() const noexcept {
            return m_frequency.load(std::memory_order_relaxed);
        }

        size_t get_due_time() const noexcept {
            return m_due_time;  // no need to synchronize, const anyway.
        }

        bool is_oneshot() const noexcept {
            return m_is_oneshot;
        }

        std::shared_ptr<executor> get_executor() const noexcept {
            return m_executor;
        }

        std::weak_ptr<timer_queue> get_timer_queue() const noexcept {
            return m_timer_queue;
        }

        void set_new_frequency(size_t new_frequency) noexcept {
            m_frequency.store(new_frequency, std::memory_order_relaxed);
        }

        void cancel() noexcept {
            m_cancelled.store(true, std::memory_order_relaxed);
        }

        bool cancelled() const noexcept {
            return m_cancelled.load(std::memory_order_relaxed);
        }
    };

    template<class callable_type>
    class timer_state final : public timer_state_base {

       private:
        callable_type m_callable;

       public:
        template<class given_callable_type>
        timer_state(size_t due_time,
                    size_t frequency,
                    std::shared_ptr<concurrencpp::executor> executor,
                    std::weak_ptr<concurrencpp::timer_queue> timer_queue,
                    bool is_oneshot,
                    given_callable_type&& callable) :
            timer_state_base(due_time, frequency, std::move(executor), std::move(timer_queue), is_oneshot),
            m_callable(std::forward<given_callable_type>(callable)) {}

        void execute() override {
            if (cancelled()) {
                return;
            }

            m_callable();
        }
    };
}  // namespace concurrencpp::details

namespace concurrencpp {
    class CRCPP_API timer {

       private:
        std::shared_ptr<details::timer_state_base> m_state;

        void throw_if_empty(const char* error_message) const;

       public:
        timer() noexcept = default;
        ~timer() noexcept;

        timer(std::shared_ptr<details::timer_state_base> timer_impl) noexcept;

        timer(timer&& rhs) noexcept = default;
        timer& operator=(timer&& rhs) noexcept;

        timer(const timer&) = delete;
        timer& operator=(const timer&) = delete;

        void cancel();

        std::chrono::milliseconds get_due_time() const;
        std::shared_ptr<executor> get_executor() const;
        std::weak_ptr<timer_queue> get_timer_queue() const;

        std::chrono::milliseconds get_frequency() const;
        void set_frequency(std::chrono::milliseconds new_frequency);

        explicit operator bool() const noexcept {
            return static_cast<bool>(m_state);
        }
    };
}  // namespace concurrencpp

#endif  // CONCURRENCPP_TIMER_H
