#ifndef CONCURRENCPP_PROMISES_H
#define CONCURRENCPP_PROMISES_H

#include "concurrencpp/coroutines/coroutine.h"
#include "concurrencpp/executors/executor_all.h"
#include "concurrencpp/results/impl/lazy_result_state.h"
#include "concurrencpp/results/impl/result_state.h"
#include "concurrencpp/results/impl/return_value_struct.h"
#include "concurrencpp/task.h"

#include <vector>

#include "constants.h"
#include "concurrencpp/errors.h"

namespace concurrencpp::details {
    template<class executor_type>
    class initialy_rescheduled_promise {

        executor_type& m_initial_executor;

        static_assert(
            std::is_base_of_v<concurrencpp::executor, executor_type>,
            "concurrencpp::initialy_rescheduled_promise<<executor_type>> - <<executor_type>> isn't driven from concurrencpp::executor.");

        static executor_type& to_ref(executor_type* executor_ptr) {
            if (executor_ptr == nullptr) {
                throw std::invalid_argument(consts::k_parallel_coroutine_null_exception_err_msg);
            }

            return *executor_ptr;
        }

       public:
        template<class... argument_types>
        initialy_rescheduled_promise(executor_tag, executor_type* executor_ptr, argument_types&&...) :
            m_initial_executor(to_ref(executor_ptr)) {}

        template<class... argument_types>
        initialy_rescheduled_promise(executor_tag, executor_type& executor_ptr, argument_types&&...) :
            m_initial_executor(executor_ptr) {}

        template<class... argument_types>
        initialy_rescheduled_promise(executor_tag, std::shared_ptr<executor_type> executor, argument_types&&... args) :
            initialy_rescheduled_promise(executor_tag {}, executor.get(), std::forward<argument_types>(args)...) {}

        template<class class_type, class... argument_types>
        initialy_rescheduled_promise(class_type&&, executor_tag, std::shared_ptr<executor_type> executor, argument_types&&... args) :
            initialy_rescheduled_promise(executor_tag {}, *executor, std::forward<argument_types>(args)...) {}

        class initial_scheduling_awaiter : public suspend_always {

           private:
            bool m_interrupted = false;

           public:
            template<class promise_type>
            void await_suspend(coroutine_handle<promise_type> handle) {
                try {
                    handle.promise().m_initial_executor.post(await_via_functor {handle, &m_interrupted});                
                } catch (...) {
                    // do nothing. ~await_via_functor will resume the coroutine and throw an exception.
                }
            }

            void await_resume() const {
                if (m_interrupted) {
                    throw errors::broken_task(consts::k_broken_task_exception_error_msg);
                }
            }
        };

        initial_scheduling_awaiter initial_suspend() const noexcept {
            return {};
        }
    };

    struct initialy_resumed_promise {
        suspend_never initial_suspend() const noexcept {
            return {};
        }
    };

    struct null_result_promise {
        null_result get_return_object() const noexcept {
            return {};
        }

        suspend_never final_suspend() const noexcept {
            return {};
        }

        void unhandled_exception() const noexcept {}
        void return_void() const noexcept {}
    };

    struct result_publisher : public suspend_always {
        template<class promise_type>
        void await_suspend(coroutine_handle<promise_type> handle) const noexcept {
            handle.promise().complete_producer(handle);
        }
    };

    template<class type>
    struct result_coro_promise : public return_value_struct<result_coro_promise<type>, type> {

       private:
        result_state<type> m_result_state;

       public:
        template<class... argument_types>
        void set_result(argument_types&&... arguments) noexcept(noexcept(type(std::forward<argument_types>(arguments)...))) {
            this->m_result_state.set_result(std::forward<argument_types>(arguments)...);
        }

        void unhandled_exception() noexcept {
            this->m_result_state.set_exception(std::current_exception());
        }

        result<type> get_return_object() noexcept {
            return {&m_result_state};
        }

        void complete_producer(coroutine_handle<void> done_handle) noexcept {
            this->m_result_state.complete_producer(done_handle);
        }

        result_publisher final_suspend() const noexcept {
            return {};
        }
    };

    template<class type>
    struct lazy_promise : lazy_result_state<type>, public return_value_struct<lazy_promise<type>, type> {};

    struct initialy_resumed_null_result_promise : public initialy_resumed_promise, public null_result_promise {};

    template<class return_type>
    struct initialy_resumed_result_promise : public initialy_resumed_promise, public result_coro_promise<return_type> {};

    template<class executor_type>
    struct initialy_rescheduled_null_result_promise : public initialy_rescheduled_promise<executor_type>, public null_result_promise {
        using initialy_rescheduled_promise<executor_type>::initialy_rescheduled_promise;
    };

    template<class return_type, class executor_type>
    struct initialy_rescheduled_result_promise :
        public initialy_rescheduled_promise<executor_type>,
        public result_coro_promise<return_type> {
        using initialy_rescheduled_promise<executor_type>::initialy_rescheduled_promise;
    };
}  // namespace concurrencpp::details

namespace CRCPP_COROUTINE_NAMESPACE {
    // No executor + No result
    template<class... arguments>
    struct coroutine_traits<::concurrencpp::null_result, arguments...> {
        using promise_type = concurrencpp::details::initialy_resumed_null_result_promise;
    };

    // No executor + result
    template<class type, class... arguments>
    struct coroutine_traits<::concurrencpp::result<type>, arguments...> {
        using promise_type = concurrencpp::details::initialy_resumed_result_promise<type>;
    };

    // Executor + no result
    template<class executor_type, class... arguments>
    struct coroutine_traits<concurrencpp::null_result, concurrencpp::executor_tag, std::shared_ptr<executor_type>, arguments...> {
        using promise_type = concurrencpp::details::initialy_rescheduled_null_result_promise<executor_type>;
    };

    template<class executor_type, class... arguments>
    struct coroutine_traits<concurrencpp::null_result, concurrencpp::executor_tag, executor_type*, arguments...> {
        using promise_type = concurrencpp::details::initialy_rescheduled_null_result_promise<executor_type>;
    };

    template<class executor_type, class... arguments>
    struct coroutine_traits<concurrencpp::null_result, concurrencpp::executor_tag, executor_type&, arguments...> {
        using promise_type = concurrencpp::details::initialy_rescheduled_null_result_promise<executor_type>;
    };

    // Executor + result
    template<class type, class executor_type, class... arguments>
    struct coroutine_traits<::concurrencpp::result<type>, concurrencpp::executor_tag, std::shared_ptr<executor_type>, arguments...> {
        using promise_type = concurrencpp::details::initialy_rescheduled_result_promise<type, executor_type>;
    };

    template<class type, class executor_type, class... arguments>
    struct coroutine_traits<::concurrencpp::result<type>, concurrencpp::executor_tag, executor_type*, arguments...> {
        using promise_type = concurrencpp::details::initialy_rescheduled_result_promise<type, executor_type>;
    };

    template<class type, class executor_type, class... arguments>
    struct coroutine_traits<::concurrencpp::result<type>, concurrencpp::executor_tag, executor_type&, arguments...> {
        using promise_type = concurrencpp::details::initialy_rescheduled_result_promise<type, executor_type>;
    };

    // Lazy
    template<class type, class... arguments>
    struct coroutine_traits<::concurrencpp::lazy_result<type>, arguments...> {
        using promise_type = concurrencpp::details::lazy_promise<type>;
    };

}  // namespace CRCPP_COROUTINE_NAMESPACE

#endif
