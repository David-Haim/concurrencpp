#ifndef CONCURRENCPP_EXECUTOR_H
#define CONCURRENCPP_EXECUTOR_H

#include "concurrencpp/errors.h"
#include "concurrencpp/executors/constants.h"
#include "concurrencpp/coroutines/coroutine.h"

#include <string>
#include <string_view>

#include <cassert>

namespace concurrencpp::details {
    [[noreturn]] CRCPP_API void throw_runtime_shutdown_exception(std::string_view executor_name);
    std::string make_executor_worker_name(std::string_view executor_name);
}  // namespace concurrencpp::details

namespace concurrencpp {
    class CRCPP_API task {

       public:
        task* next = nullptr;
        task* prev = nullptr;

       private:
        details::coroutine_handle<void> coro_handle;
        bool m_interrupted = false;

       public:
        void set_coroutine_handle(details::coroutine_handle<void> handle) noexcept {
            coro_handle = handle;
        }

        void resume() noexcept {
            assert(static_cast<bool>(coro_handle));
            coro_handle();
        }

        void interrupt() noexcept {
            m_interrupted = true;

            assert(static_cast<bool>(coro_handle));
            coro_handle();
        }

        bool interrupted() const noexcept {
            return m_interrupted;
        }

        void throw_if_interrupted() const {
            if (m_interrupted) {
                throw errors::broken_task(details::consts::k_broken_task_exception_error_msg);
            }
        }
    };

    struct CRCPP_API executor {
        const std::string name;

        executor(std::string_view name) : name(name) {}

        virtual ~executor() noexcept = default;

        virtual void enqueue(task& task) = 0;

        virtual int max_concurrency_level() const noexcept = 0;

        virtual bool shutdown_requested() const = 0;
        virtual void shutdown() = 0;
    };
}  // namespace concurrencpp

#endif
