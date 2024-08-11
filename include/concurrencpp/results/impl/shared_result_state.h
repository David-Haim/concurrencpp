#ifndef CONCURRENCPP_SHARED_RESULT_STATE_H
#define CONCURRENCPP_SHARED_RESULT_STATE_H

#include "concurrencpp/platform_defs.h"
#include "concurrencpp/results/impl/result_state.h"

#include <atomic>

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

        static shared_await_context* result_ready_constant() noexcept;

       public:
        virtual ~shared_result_state_base() noexcept = default;

        virtual void on_result_finished() noexcept = 0;

        result_status status() const noexcept;

        void wait() noexcept;

        bool await(shared_await_context& awaiter) noexcept;

        template<class duration_unit, class ratio>
        result_status wait_for(const std::chrono::duration<duration_unit, ratio>& duration) {
            const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration) + std::chrono::milliseconds(2);
            atomic_wait_for(m_status, result_status::idle, ms, std::memory_order_acquire);
            return status();
        }

        template<class clock, class duration>
        result_status wait_until(const std::chrono::time_point<clock, duration>& timeout_time) {
            const auto time_now = clock::now();
            if (time_now >= timeout_time) {
                return status();
            }

            return wait_for(timeout_time - time_now);
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
            atomic_notify_all(m_status);

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