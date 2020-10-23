#ifndef CONCURRENCPP_PROMISES_H
#define CONCURRENCPP_PROMISES_H

#include "concurrencpp/results/result_core.h"

#include "concurrencpp/errors.h"

#include <vector>

namespace concurrencpp::details {
    struct coroutine_per_thread_data {
        executor* executor = nullptr;
        std::vector<std::experimental::coroutine_handle<>>* accumulator = nullptr;

        static thread_local coroutine_per_thread_data s_tl_per_thread_data;
    };

    template<class executor_type>
    struct initial_scheduling_awaiter : public std::experimental::suspend_always {
        void await_suspend(std::experimental::coroutine_handle<> handle) const {
            auto& per_thread_data = coroutine_per_thread_data::s_tl_per_thread_data;
            auto executor_base_ptr = std::exchange(per_thread_data.executor, nullptr);

            assert(executor_base_ptr != nullptr);
            assert(dynamic_cast<executor_type*>(executor_base_ptr) != nullptr);

            auto executor_ptr = static_cast<executor_type*>(executor_base_ptr);
            executor_ptr->enqueue(handle);
        }
    };

    template<>
    struct initial_scheduling_awaiter<concurrencpp::inline_executor> : public std::experimental::suspend_never {};

    struct initial_accumulating_awaiter : public std::experimental::suspend_always {
        void await_suspend(std::experimental::coroutine_handle<> handle) const noexcept;
    };

    template<class executor_type>
    struct initialy_rescheduled_promise {

        static_assert(std::is_base_of_v<concurrencpp::executor, executor_type>,
                      "concurrencpp::initialy_rescheduled_promise<<executor_type>> - <<executor_type>> isn't driven from concurrencpp::executor.");

        template<class... argument_types>
        static void* operator new(size_t size, argument_types&&...) {
            return ::operator new(size);
        }

        template<class... argument_types>
        static void* operator new(size_t size, executor_tag, executor_type* executor_ptr, argument_types&&...) {
            assert(executor_ptr != nullptr);
            assert(coroutine_per_thread_data::s_tl_per_thread_data.executor == nullptr);
            coroutine_per_thread_data::s_tl_per_thread_data.executor = executor_ptr;

            return ::operator new(size);
        }

        template<class... argument_types>
        static void* operator new(size_t size, executor_tag, std::shared_ptr<executor_type> executor, argument_types&&... args) {
            return operator new (size, executor_tag {}, executor.get(), std::forward<argument_types>(args)...);
        }

        template<class... argument_types>
        static void* operator new(size_t size, executor_tag, executor_type& executor, argument_types&&... args) {

            return operator new (size, executor_tag {}, std::addressof(executor), std::forward<argument_types>(args)...);
        }

        initial_scheduling_awaiter<executor_type> initial_suspend() const noexcept {
            return {};
        }
    };

    struct initialy_resumed_promise {
        std::experimental::suspend_never initial_suspend() const noexcept {
            return {};
        }
    };

    struct bulk_promise {
        template<class... argument_types>
        static void* operator new(size_t size, argument_types&&...) {
            return ::operator new(size);
        }

        template<class... argument_types>
        static void* operator new(size_t size, executor_bulk_tag, std::vector<std::experimental::coroutine_handle<>>* accumulator, argument_types&&...) {

            assert(accumulator != nullptr);
            assert(coroutine_per_thread_data::s_tl_per_thread_data.accumulator == nullptr);
            coroutine_per_thread_data::s_tl_per_thread_data.accumulator = accumulator;

            return ::operator new(size);
        }

        initial_accumulating_awaiter initial_suspend() const noexcept {
            return {};
        }
    };

    struct null_result_promise {
        null_result get_return_object() const noexcept {
            return {};
        }

        std::experimental::suspend_never final_suspend() const noexcept {
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

    struct result_publisher : public std::experimental::suspend_always {
        template<class promise_type>
        bool await_suspend(std::experimental::coroutine_handle<promise_type> handle) const noexcept {
            handle.promise().publish_result();
            return false;  // don't suspend, resume and destroy this
        }
    };

    template<class type>
    struct result_coro_promise : public return_value_struct<result_coro_promise<type>, type> {

       private:
        std::shared_ptr<result_core<type>> m_result_ptr;

       public:
        result_coro_promise() : m_result_ptr(std::make_shared<result_core<type>>()) {}

        ~result_coro_promise() noexcept {
            if (!static_cast<bool>(this->m_result_ptr)) {
                return;
            }

            auto broken_task_error = std::make_exception_ptr(concurrencpp::errors::broken_task("coroutine was destroyed before finishing execution"));
            this->m_result_ptr->set_exception(broken_task_error);
            this->m_result_ptr->publish_result();
        }

        template<class... argument_types>
        void set_result(argument_types&&... args) {
            this->m_result_ptr->set_result(std::forward<argument_types>(args)...);
        }

        void unhandled_exception() noexcept {
            this->m_result_ptr->set_exception(std::current_exception());
        }

        ::concurrencpp::result<type> get_return_object() noexcept {
            return {this->m_result_ptr};
        }

        void publish_result() noexcept {
            this->m_result_ptr->publish_result();
            this->m_result_ptr.reset();
        }

        result_publisher final_suspend() const noexcept {
            return {};
        }
    };

    struct initialy_resumed_null_result_promise : public initialy_resumed_promise, public null_result_promise {};

    template<class return_type>
    struct initialy_resumed_result_promise : public initialy_resumed_promise, public result_coro_promise<return_type> {};

    template<class executor_type>
    struct initialy_rescheduled_null_result_promise : public initialy_rescheduled_promise<executor_type>, public null_result_promise {};

    template<class return_type, class executor_type>
    struct initialy_rescheduled_result_promise : public initialy_rescheduled_promise<executor_type>, public result_coro_promise<return_type> {};

    struct bulk_null_result_promise : public bulk_promise, public null_result_promise {};

    template<class return_type>
    struct bulk_result_promise : public bulk_promise, public result_coro_promise<return_type> {};
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
                            std::vector<std::experimental::coroutine_handle<>>*,
                            arguments...> {
        using promise_type = concurrencpp::details::bulk_null_result_promise;
    };

    // Bulk + result
    template<class type, class... arguments>
    struct coroutine_traits<::concurrencpp::result<type>,
                            concurrencpp::details::executor_bulk_tag,
                            std::vector<std::experimental::coroutine_handle<>>*,
                            arguments...> {
        using promise_type = concurrencpp::details::bulk_result_promise<type>;
    };
}  // namespace std::experimental

#endif
