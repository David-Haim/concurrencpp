#ifndef CONCURRENCPP_SHARED_RESULT_STATE_H
#define CONCURRENCPP_SHARED_RESULT_STATE_H

#include "concurrencpp/results/impl/result_state.h"

#include <shared_mutex>

namespace concurrencpp::details {
    struct shared_await_context {
        shared_await_context* next = nullptr;
        coroutine_handle<void> caller_handle;
    };

    struct shared_await_via_context {
        shared_await_via_context* next = nullptr;
        await_via_context await_context;

        shared_await_via_context(const std::shared_ptr<executor>& executor) noexcept;
    };
}  // namespace concurrencpp::details

namespace concurrencpp::details {
    class shared_result_state_base {

       protected:
        mutable std::shared_mutex m_lock;
        shared_await_context* m_awaiters = nullptr;
        shared_await_via_context* m_via_awaiters = nullptr;
        std::condition_variable_any m_condition;
        bool m_ready = false;

        void await_impl(std::unique_lock<std::shared_mutex>& write_lock, shared_await_context& awaiter) noexcept;
        void await_via_impl(std::unique_lock<std::shared_mutex>& write_lock, shared_await_via_context& awaiter) noexcept;
        void wait_impl(std::unique_lock<std::shared_mutex>& write_lock) noexcept;
        bool wait_for_impl(std::unique_lock<std::shared_mutex>& write_lock, std::chrono::milliseconds ms) noexcept;

        void notify_all(std::unique_lock<std::shared_mutex>& lock) noexcept;

       public:
        virtual ~shared_result_state_base() noexcept = default;
        virtual void on_result_ready() noexcept = 0;
    };

    template<class type>
    class shared_result_state final : public shared_result_state_base {

       private:
        consumer_result_state_ptr<type> m_state;
        producer_context<type> m_producer_context;

        void on_result_ready() noexcept override {
            std::unique_lock<std::shared_mutex> lock(m_lock);
            auto state = std::move(m_state);
            state->initialize_producer_from(m_producer_context);
            notify_all(lock);
        }

       public:
        shared_result_state(consumer_result_state_ptr<type> state) : m_state(std::move(state)) {}

        ~shared_result_state() noexcept {
            std::unique_lock<std::shared_mutex> lock(m_lock);
            auto state = std::move(m_state);
            lock.unlock();

            if (static_cast<bool>(state)) {
                state->try_rewind_consumer();
            }
        }

        result_status status() const noexcept {
            std::shared_lock<std::shared_mutex> lock(m_lock);
            return m_producer_context.status();
        }

        bool await(shared_await_context& awaiter) noexcept {
            {
                std::shared_lock<std::shared_mutex> read_lock(m_lock);
                if (m_ready) {
                    read_lock.unlock();
                    return false;
                }
            }

            std::unique_lock<std::shared_mutex> write_lock(m_lock);
            if (m_ready) {
                write_lock.unlock();
                return false;
            }

            await_impl(write_lock, awaiter);
            write_lock.unlock();

            return true;
        }

        bool await_via(shared_await_via_context& awaiter, const bool force_rescheduling) noexcept {
            auto resume_if_ready = [&awaiter, force_rescheduling]() mutable {
                if (force_rescheduling) {
                    awaiter.await_context();
                }

                return force_rescheduling;
            };

            {
                std::shared_lock<std::shared_mutex> read_lock(m_lock);
                if (m_ready) {
                    read_lock.unlock();
                    return resume_if_ready();
                }
            }

            std::unique_lock<std::shared_mutex> write_lock(m_lock);
            if (m_ready) {
                write_lock.unlock();
                return resume_if_ready();
            }

            await_via_impl(write_lock, awaiter);
            write_lock.unlock();
            return true;
        }

        void wait() noexcept {
            {
                std::shared_lock<std::shared_mutex> read_lock(m_lock);
                if (m_ready) {
                    read_lock.unlock();
                    return;
                }
            }

            std::unique_lock<std::shared_mutex> write_lock(m_lock);
            if (m_ready) {
                write_lock.unlock();
                return;
            }

            wait_impl(write_lock);
        }

        template<class duration_unit, class ratio>
        result_status wait_for(std::chrono::duration<duration_unit, ratio> duration) noexcept {
            {
                std::shared_lock<std::shared_mutex> read_lock(m_lock);
                if (m_ready) {
                    return m_producer_context.status();
                }
            }

            const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration) + std::chrono::milliseconds(1);

            std::unique_lock<std::shared_mutex> lock(m_lock);
            if (m_ready) {
                return m_producer_context.status();
            }

            const auto ready = wait_for_impl(lock, ms);
            if (ready) {
                return m_producer_context.status();
            }

            lock.unlock();
            return result_status::idle;
        }

        template<class clock, class duration>
        result_status wait_until(const std::chrono::time_point<clock, duration>& timeout_time) noexcept {
            const auto now = clock::now();
            if (timeout_time <= now) {
                return status();
            }

            const auto diff = timeout_time - now;
            return wait_for(diff);
        }

        std::add_lvalue_reference_t<type> get() {
            return m_producer_context.get_ref();
        }
    };
}  // namespace concurrencpp::details

#endif
