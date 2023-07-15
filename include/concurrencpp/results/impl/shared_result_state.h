#ifndef CONCURRENCPP_SHARED_RESULT_STATE_H
#define CONCURRENCPP_SHARED_RESULT_STATE_H

#include "concurrencpp/platform_defs.h"
#include "concurrencpp/results/impl/result_state.h"

#include <atomic>
#include <semaphore>

#include <cassert>

namespace concurrencpp::details {
    struct shared_result_helper {
        template<class type>
        static consumer_result_state_ptr<type> get_state(result<type>& result) noexcept {
            return std::move(result.m_state);
        }
    };

    struct CRCPP_API shared_await_context {
        shared_await_context* next = nullptr;
        coroutine_handle<void> caller_handle;
    };

    class CRCPP_API shared_result_state_base {

       protected:
        std::atomic<result_status> m_status {result_status::idle};
        std::atomic<shared_await_context*> m_awaiters {nullptr};
        std::counting_semaphore<> m_semaphore {0};

        static shared_await_context* result_ready_constant() noexcept;

       public:
        virtual ~shared_result_state_base() noexcept = default;

        virtual void on_result_finished() noexcept = 0;

        result_status status() const noexcept;

        void wait() noexcept;

        bool await(shared_await_context& awaiter) noexcept;

        template<class duration_unit, class ratio>
        result_status wait_for(std::chrono::duration<duration_unit, ratio> duration) {
            const auto time_point = std::chrono::system_clock::now() + duration;
            return wait_until(time_point);
        }

        template<class clock, class duration>
        result_status wait_until(const std::chrono::time_point<clock, duration>& timeout_time) {
            while ((status() == result_status::idle) && (clock::now() < timeout_time)) {
                const auto res = m_semaphore.try_acquire_until(timeout_time);
                (void)res;
            }

            return status();
        }
    };

    template<class type>
    class shared_result_state final : public shared_result_state_base {

       private:
        consumer_result_state_ptr<type> m_result_state;

       public:
        shared_result_state(consumer_result_state_ptr<type> result_state) noexcept : m_result_state(std::move(result_state)) {
            assert(static_cast<bool>(m_result_state));
        }

        ~shared_result_state() noexcept {
            assert(static_cast<bool>(m_result_state));
            m_result_state->try_rewind_consumer();
            m_result_state.reset();
        }

        void share(const std::shared_ptr<shared_result_state_base>& self) noexcept {
            assert(static_cast<bool>(m_result_state));
            m_result_state->share(self);
        }

        std::add_lvalue_reference_t<type> get() {
            assert(static_cast<bool>(m_result_state));
            return m_result_state->get_ref();
        }

        void on_result_finished() noexcept override {
            m_status.store(m_result_state->status(), std::memory_order_release);
            m_status.notify_all();

            /* theoretically buggish, practically there's no way
               that we'll have more than max(ptrdiff_t) / 2 waiters.
               on 64 bits, that's 2^62 waiters, on 32 bits thats 2^30 waiters.
               memory will run out before enough tasks could be created to wait this synchronously
            */
            m_semaphore.release(m_semaphore.max() / 2);

            auto k_result_ready = result_ready_constant();
            auto awaiters = m_awaiters.exchange(k_result_ready, std::memory_order_acq_rel);

            shared_await_context* current = awaiters;
            shared_await_context *prev = nullptr, *next = nullptr;

            while (current != nullptr) {
                next = current->next;
                current->next = prev;
                prev = current;
                current = next;
            }

            awaiters = prev;

            while (awaiters != nullptr) {
                assert(static_cast<bool>(awaiters->caller_handle));
                auto caller_handle = awaiters->caller_handle;
                awaiters = awaiters->next;
                caller_handle();
            }
        }
    };
}  // namespace concurrencpp::details

#endif