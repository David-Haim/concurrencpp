#ifndef CONCURRENCPP_SHARED_RESULT_STATE_H
#define CONCURRENCPP_SHARED_RESULT_STATE_H

#include <optional>

#include "consumer_context.h"

namespace concurrencpp::details {
    struct shared_await_context {
        shared_await_context* next = nullptr;
        await_context await_context;
    };

    struct shared_await_via_context {
        shared_await_via_context* next = nullptr;
        await_via_context await_context;

        shared_await_via_context(std::shared_ptr<concurrencpp::executor> executor) noexcept;
    };

    class shared_result_state_base {

       protected:
        std::mutex m_lock;
        shared_await_context* m_awaiters = nullptr;
        shared_await_via_context* m_via_awaiters = nullptr;
        std::condition_variable m_condition;
        bool m_ready = false;

       public:
        void await_impl(shared_await_context& awaiter) noexcept;
        void await_via_impl(shared_await_via_context& awaiter) noexcept;
        void wait_impl(std::unique_lock<std::mutex>& lock);
        bool wait_for_impl(std::unique_lock<std::mutex>& lock, std::chrono::milliseconds ms);

        void notify_all() noexcept;
    };

    template<class type>
    class shared_result_state : public shared_result_state_base {

       private:
        std::shared_ptr<result_state<type>> m_state;

        bool result_ready() const noexcept {
            return m_state->is_ready();
        }

       public:
        shared_result_state(std::shared_ptr<result_state<type>> state) : m_state(std::move(state)) {}

        result_status status() const noexcept {
            return m_state->status_shared();
        }

        bool await(shared_await_context& awaiter) noexcept {
            if (result_ready()) {
                return false;
            }

            std::unique_lock<std::mutex> lock(m_lock);
            if (result_ready()) {
                return false;
            }

            await_impl(awaiter);
            return true;
        }

        bool await_via(shared_await_via_context& awaiter, const bool force_rescheduling) noexcept {
            if (result_ready()) {
                if (force_rescheduling) {
                    awaiter.await_context();
                    return true;
                }

                return false;
            }

            std::unique_lock<std::mutex> lock(m_lock);
            if (result_ready()) {
                if (force_rescheduling) {
                    awaiter.await_context();
                    return true;
                }
                return false;
            }

            await_via_impl(awaiter);
            return true;
        }

        void wait() {
            if (result_ready()) {
                return;
            }

            std::unique_lock<std::mutex> lock(m_lock);
            if (result_ready()) {
                return;
            }

            wait_impl(lock);
        }

        template<class duration_unit, class ratio>
        concurrencpp::result_status wait_for(std::chrono::duration<duration_unit, ratio> duration) {
            const auto status0 = status();
            if (status0 != result_status::idle) {
                return status0;
            }

            std::unique_lock<std::mutex> lock(m_lock);
            const auto status1 = status();
            if (status1 != result_status::idle) {
                return status1;
            }

            const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration) + std::chrono::milliseconds(1);
            const auto ready = wait_for_impl(lock, ms);
            lock.unlock();
            return ready ? status() : result_status::idle;
        }

        template<class clock, class duration>
        concurrencpp::result_status wait_until(const std::chrono::time_point<clock, duration>& timeout_time) {
            const auto now = clock::now();
            if (timeout_time <= now) {
                return status();
            }

            const auto diff = timeout_time - now;
            return wait_for(diff);
        }

        std::add_lvalue_reference_t<type> get() {
            return m_state->get_ref();
        }
    };
}  // namespace concurrencpp::details

#endif
