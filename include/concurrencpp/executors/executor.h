#ifndef CONCURRENCPP_EXECUTOR_H
#define CONCURRENCPP_EXECUTOR_H

#include "concurrencpp/errors.h"
#include "concurrencpp/results/result.h"
#include "concurrencpp/executors/constants.h"
#include "concurrencpp/coroutines/coroutine.h"

#include <string>
#include <string_view>

#include <cassert>

namespace concurrencpp {
    class CRCPP_API task {

       public:
        task* next = nullptr;
        task* prev = nullptr;

       private:
        details::coroutine_handle<void> m_handle;
        bool m_interrupted = false;

       public:
        task() noexcept = default;

        task(const task&) noexcept = delete;
        task(task&&) noexcept = delete;

        task& operator=(const task&) noexcept = delete;
        task& operator=(task&&) noexcept = delete;

        void set_coroutine_handle(details::coroutine_handle<void> handle) noexcept {
            if (static_cast<bool>(handle)) {
                assert(!handle.done());
                m_handle = handle;
            }
        }

        void resume() noexcept {
            if (static_cast<bool>(m_handle)) {
                m_handle();
            }
        }

        void interrupt() noexcept {
            m_interrupted = true;
            resume();
        }

        void throw_if_interrupted() const {
            if (m_interrupted) {
                throw errors::broken_task(details::consts::k_broken_task_exception_error_msg);
            }
        }
    };

    class CRCPP_API executor {

       private:
        template<class callable_type, class... argument_types>
        static null_result post_impl(executor_tag, executor&, callable_type callable, argument_types... arguments) {
            callable(arguments...);
            co_return;
        }

        template<class return_type, class callable_type, class... argument_types>
        static result<return_type> submit_impl(executor_tag, executor&, callable_type callable, argument_types... arguments) {
            co_return callable(arguments...);
        }

       protected:
        [[noreturn]] void throw_runtime_shutdown_exception();
        std::string make_executor_worker_name();

       public:
        const std::string name;

        executor(std::string_view name) : name(name) {}

        virtual ~executor() noexcept = default;

        virtual void enqueue(task& task) = 0;

        virtual int max_concurrency_level() const noexcept = 0;

        virtual bool shutdown_requested() const = 0;
        virtual void shutdown() = 0;

        template<class callable_type, class... argument_types>
        null_result post(callable_type&& callable, argument_types&&... arguments) {
            return post_impl({}, *this, std::forward<callable_type>(callable), std::forward<argument_types>(arguments)...);
        }

        template<class callable_type, class... argument_types>
        auto submit(callable_type&& callable, argument_types&&... arguments) {
            using return_type = typename std::invoke_result_t<callable_type, argument_types...>;
            return submit_impl<return_type>({},
                                            *this,
                                            std::forward<callable_type>(callable),
                                            std::forward<argument_types>(arguments)...);
        }
    };
}  // namespace concurrencpp

#endif