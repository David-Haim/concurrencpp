#ifndef CONCURRENCPP_SHARED_RESULT_STATE_H
#define CONCURRENCPP_SHARED_RESULT_STATE_H

#include "concurrencpp/coroutines/coroutine.h"
#include "concurrencpp/forward_declarations.h"
#include "concurrencpp/results/impl/producer_context.h"
#include "concurrencpp/results/impl/return_value_struct.h"

#include <atomic>
#include <mutex>
#include <optional>
#include <condition_variable>

#include <cassert>

namespace concurrencpp::details {
    struct shared_await_context {
        shared_await_context* next = nullptr;
        coroutine_handle<void> caller_handle;
    };
}  // namespace concurrencpp::details

namespace concurrencpp::details {
    class shared_result_state_base {

       protected:
        std::atomic_bool m_ready {false};
        mutable std::mutex m_lock;
        shared_await_context* m_awaiters = nullptr;
        std::optional<std::condition_variable> m_condition;

        void await_impl(std::unique_lock<std::mutex>& lock, shared_await_context& awaiter) noexcept;
        void wait_impl(std::unique_lock<std::mutex>& lock);
        bool wait_for_impl(std::unique_lock<std::mutex>& lock, std::chrono::milliseconds ms);

       public:
        void complete_producer();
        bool await(shared_await_context& awaiter);
        void wait();
    };

    template<class type>
    class shared_result_state final : public shared_result_state_base {

       private:
        producer_context<type> m_producer;

        void assert_done() const noexcept {
            assert(m_ready.load(std::memory_order_acquire));
            assert(m_producer.status() != result_status::idle);
        }

       public:
        result_status status() const noexcept {
            if (!m_ready.load(std::memory_order_acquire)) {
                return result_status::idle;
            }

            return m_producer.status();
        }

        template<class duration_unit, class ratio>
        result_status wait_for(std::chrono::duration<duration_unit, ratio> duration) {
            if (m_ready.load(std::memory_order_acquire)) {
                return m_producer.status();
            }

            const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration) + std::chrono::milliseconds(1);

            std::unique_lock<std::mutex> lock(m_lock);
            if (m_ready.load(std::memory_order_acquire)) {
                return m_producer.status();
            }

            const auto ready = wait_for_impl(lock, ms);
            if (ready) {
                assert_done();
                return m_producer.status();
            }

            lock.unlock();
            return result_status::idle;
        }

        template<class clock, class duration>
        result_status wait_until(const std::chrono::time_point<clock, duration>& timeout_time) {
            const auto now = clock::now();
            if (timeout_time <= now) {
                return status();
            }

            const auto diff = timeout_time - now;
            return wait_for(diff);
        }

        std::add_lvalue_reference_t<type> get() {
            return m_producer.get_ref();
        }

        template<class... argument_types>
        void set_result(argument_types&&... arguments) noexcept(noexcept(type(std::forward<argument_types>(arguments)...))) {
            m_producer.build_result(std::forward<argument_types>(arguments)...);
        }

        void unhandled_exception() noexcept {
            m_producer.build_exception(std::current_exception());
        }
    };
}  // namespace concurrencpp::details

namespace concurrencpp::details {
    struct shared_result_publisher : public suspend_always {
        template<class promise_type>
        bool await_suspend(coroutine_handle<promise_type> handle) const noexcept {
            // TODO : this can (very rarely) throw, but the standard mandates us to have a noexcept finalizer
            handle.promise().complete_producer();
            return false;
        }
    };

    template<class type>
    class shared_result_promise : public return_value_struct<shared_result_promise<type>, type> {

       private:
        const std::shared_ptr<shared_result_state<type>> m_state = std::make_shared<shared_result_state<type>>();

       public:
        template<class... argument_types>
        void set_result(argument_types&&... arguments) noexcept(noexcept(type(std::forward<argument_types>(arguments)...))) {
            m_state->set_result(std::forward<argument_types>(arguments)...);
        }

        void unhandled_exception() const noexcept {
            m_state->unhandled_exception();
        }

        shared_result<type> get_return_object() noexcept {
            return shared_result<type> {m_state};
        }

        suspend_never initial_suspend() const noexcept {
            return {};
        }

        shared_result_publisher final_suspend() const noexcept {
            return {};
        }

        void complete_producer() const {
            m_state->complete_producer();
        }
    };

    struct shared_result_tag {};
}  // namespace concurrencpp::details

namespace std::experimental {
    // No executor + No result
    template<class type>
    struct coroutine_traits<::concurrencpp::shared_result<type>,
                            concurrencpp::details::shared_result_tag,
                            concurrencpp::result<type>> {
        using promise_type = concurrencpp::details::shared_result_promise<type>;
    };
}  // namespace std::experimental

#endif
