#ifndef CONCURRENCPP_SHARED_RESULT_STATE_H
#define CONCURRENCPP_SHARED_RESULT_STATE_H

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
        static constexpr std::ptrdiff_t k_max_waiters = std::numeric_limits<std::ptrdiff_t>::max();

        std::atomic<shared_await_context*> m_awaiters {nullptr};
        std::counting_semaphore<k_max_waiters> m_semaphore {0};

        static shared_await_context* result_ready_constant() noexcept;
        bool result_ready() const noexcept;

       public:
        bool await(shared_await_context& awaiter) noexcept;

        void on_result_finished() noexcept;
    };

    template<class type>
    class CRCPP_API shared_result_state final : public shared_result_state_base {

       private:
        consumer_result_state_ptr<type> m_result_state;

       public:
        shared_result_state(consumer_result_state_ptr<type> result_state) noexcept : m_result_state(std::move(result_state)) {
            assert(static_cast<bool>(m_result_state));
            m_result_state->share(*this);
        }

        ~shared_result_state() noexcept {
            m_result_state->complete_consumer();
        }

        result_status status() const noexcept {
            if (!result_ready()) {
                return result_status::idle;
            }

            assert(static_cast<bool>(m_result_state));
            return m_result_state->status();
        }

        void wait() noexcept {
            if (!result_ready()) {
                m_semaphore.acquire();
            }
        }

        template<class duration_unit, class ratio>
        result_status wait_for(std::chrono::duration<duration_unit, ratio> duration) {
            const auto time_point = std::chrono::system_clock::now() + duration;
            return wait_until(time_point);
        }

        template<class clock, class duration>
        result_status wait_until(const std::chrono::time_point<clock, duration>& timeout_time) {
            while (!result_ready() && clock::now() < timeout_time) {
                m_semaphore.try_acquire_until(timeout_time);
            }

            assert(static_cast<bool>(m_result_state));
            return m_result_state->status();
        }

        std::add_lvalue_reference_t<type> get() {
            assert(static_cast<bool>(m_result_state));
            return m_result_state->get_ref();
        }
    };
}  // namespace concurrencpp::details

#endif