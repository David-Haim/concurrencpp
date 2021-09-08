#ifndef CONCURRENCPP_LAZY_RESULT_STATE_H
#define CONCURRENCPP_LAZY_RESULT_STATE_H

#include "concurrencpp/coroutines/coroutine.h"
#include "concurrencpp/results/impl/producer_context.h"
#include "concurrencpp/results/result_fwd_declarations.h"

namespace concurrencpp::details {
    struct lazy_final_awaiter : public suspend_always {
        template<class promise_type>
        coroutine_handle<void> await_suspend(coroutine_handle<promise_type> handle) noexcept {
            return handle.promise().resume_caller();
        }
    };

    class lazy_result_state_base {

       protected:
        coroutine_handle<void> m_caller_handle;

       public:
        coroutine_handle<void> resume_caller() const noexcept {
            return m_caller_handle;
        }

        coroutine_handle<void> await(coroutine_handle<void> caller_handle) noexcept {
            m_caller_handle = caller_handle;
            return coroutine_handle<lazy_result_state_base>::from_promise(*this);
        }
    };

    template<class type>
    class lazy_result_state : public lazy_result_state_base {

       private:
        producer_context<type> m_producer;

       public:
        result_status status() const noexcept {
            return m_producer.status();
        }

        lazy_result<type> get_return_object() noexcept {
            const auto self_handle = coroutine_handle<lazy_result_state>::from_promise(*this);
            return lazy_result<type>(self_handle);
        }

        void unhandled_exception() noexcept {
            m_producer.build_exception(std::current_exception());
        }

        suspend_always initial_suspend() const noexcept {
            return {};
        }

        lazy_final_awaiter final_suspend() const noexcept {
            return {};
        }

        template<class... argument_types>
        void set_result(argument_types&&... arguments) noexcept(noexcept(type(std::forward<argument_types>(arguments)...))) {
            m_producer.build_result(std::forward<argument_types>(arguments)...);
        }

        type get() {
            return m_producer.get();
        }
    };
}  // namespace concurrencpp::details

#endif