#ifndef CONCURRENCPP_PROMISES_H
#define CONCURRENCPP_PROMISES_H

#include "concurrencpp/task.h"
#include "concurrencpp/coroutines/coroutine.h"
#include "concurrencpp/results/impl/result_state.h"

#include <vector>

namespace concurrencpp::details {
    struct coroutine_per_thread_data {
        executor* executor = nullptr;
        std::vector<task>* accumulator = nullptr;

        static thread_local coroutine_per_thread_data s_tl_per_thread_data;
    };

    template<class executor_type>
    class initial_scheduling_awaiter : public suspend_always {

       private:
        await_via_context m_await_via_context;

       public:
        void await_suspend(coroutine_handle<void> handle) {
            m_await_via_context.set_coro_handle(handle);

            auto& per_thread_data = coroutine_per_thread_data::s_tl_per_thread_data;
            auto executor_base_ptr = std::exchange(per_thread_data.executor, nullptr);

            assert(executor_base_ptr != nullptr);
            assert(dynamic_cast<executor_type*>(executor_base_ptr) != nullptr);

            auto executor_ptr = static_cast<executor_type*>(executor_base_ptr);
            executor_ptr->enqueue(m_await_via_context.get_functor());
        }

        void await_resume() const {
            m_await_via_context.throw_if_interrupted();
        }
    };

    template<>
    struct initial_scheduling_awaiter<inline_executor> : public suspend_never {};

    class initial_accumulating_awaiter : public suspend_always {

       private:
        await_via_context m_await_via_context;

       public:
        void await_suspend(coroutine_handle<void> handle) noexcept;
        void await_resume();
    };

    template<class executor_type>
    struct initialy_rescheduled_promise {

        static_assert(
            std::is_base_of_v<concurrencpp::executor, executor_type>,
            "concurrencpp::initialy_rescheduled_promise<<executor_type>> - <<executor_type>> isn't driven from concurrencpp::executor.");

        template<class... argument_types>
        initialy_rescheduled_promise(executor_tag, executor_type* executor_ptr, argument_types&&...) {
            assert(executor_ptr != nullptr);
            assert(coroutine_per_thread_data::s_tl_per_thread_data.executor == nullptr);
            coroutine_per_thread_data::s_tl_per_thread_data.executor = executor_ptr;
        }

        template<class... argument_types>
        initialy_rescheduled_promise(executor_tag, std::shared_ptr<executor_type> executor, argument_types&&... args) :
            initialy_rescheduled_promise(executor_tag {}, executor.get(), std::forward<argument_types>(args)...) {}

        template<class... argument_types>
        initialy_rescheduled_promise(executor_tag, executor_type& executor, argument_types&&... args) :
            initialy_rescheduled_promise(executor_tag {}, std::addressof(executor), std::forward<argument_types>(args)...) {}

        initial_scheduling_awaiter<executor_type> initial_suspend() const noexcept {
            return {};
        }
    };

    struct initialy_resumed_promise {
        suspend_never initial_suspend() const noexcept {
            return {};
        }
    };

    struct bulk_promise {
        template<class... argument_types>
        bulk_promise(executor_bulk_tag, std::vector<concurrencpp::task>& accumulator, argument_types&&...) {
            assert(coroutine_per_thread_data::s_tl_per_thread_data.accumulator == nullptr);
            coroutine_per_thread_data::s_tl_per_thread_data.accumulator = &accumulator;
        }

        initial_accumulating_awaiter initial_suspend() const noexcept {
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

    template<class derived_type, class type>
    struct return_value_struct {
        template<class return_type>
        void return_value(return_type&& value) {
            auto self = static_cast<derived_type*>(this);
            self->set_result(std::forward<return_type>(value));
        }
    };

    template<class derived_type>
    struct return_value_struct<derived_type, void> {
        void return_void() noexcept {
            auto self = static_cast<derived_type*>(this);
            self->set_result();
        }
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

    struct bulk_null_result_promise : public bulk_promise, public null_result_promise {
        using bulk_promise::bulk_promise;
    };

    template<class return_type>
    struct bulk_result_promise : public bulk_promise, public result_coro_promise<return_type> {
        using bulk_promise::bulk_promise;
    };
}  // namespace concurrencpp::details

namespace std::experimental {
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

    // Bulk + no result
    template<class... arguments>
    struct coroutine_traits<::concurrencpp::null_result,
                            concurrencpp::details::executor_bulk_tag,
                            std::vector<concurrencpp::task>&,
                            arguments...> {
        using promise_type = concurrencpp::details::bulk_null_result_promise;
    };

    // Bulk + result
    template<class type, class... arguments>
    struct coroutine_traits<::concurrencpp::result<type>,
                            concurrencpp::details::executor_bulk_tag,
                            std::vector<concurrencpp::task>&,
                            arguments...> {
        using promise_type = concurrencpp::details::bulk_result_promise<type>;
    };
}  // namespace std::experimental

#endif
