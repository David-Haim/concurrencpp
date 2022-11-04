#ifndef CONCURRENCPP_RESUME_ON_H
#define CONCURRENCPP_RESUME_ON_H

#include "concurrencpp/executors/executor.h"
#include "concurrencpp/results/impl/consumer_context.h"

#include <type_traits>

namespace concurrencpp::details {
    template<class executor_type>
    class resume_on_awaitable : public suspend_always {

       private:
        executor_type& m_executor;
        bool m_interrupted = false;

       public:
        resume_on_awaitable(executor_type& executor) noexcept : m_executor(executor) {}

        resume_on_awaitable(const resume_on_awaitable&) = delete;
        resume_on_awaitable(resume_on_awaitable&&) = delete;

        resume_on_awaitable& operator=(const resume_on_awaitable&) = delete;
        resume_on_awaitable& operator=(resume_on_awaitable&&) = delete;

        void await_suspend(coroutine_handle<void> handle) {
            try {
                m_executor.post(await_via_functor {handle, &m_interrupted});
            } catch (...) {
                // the exception caused the enqeueud task to be broken and resumed with an interrupt, no need to do anything here.
            }
        }

        void await_resume() const {
            if (m_interrupted) {
                throw errors::broken_task(consts::k_broken_task_exception_error_msg);
            }
        }
    };
}  // namespace concurrencpp::details

namespace concurrencpp {
    template<class executor_type>
    auto resume_on(std::shared_ptr<executor_type> executor) {
        static_assert(std::is_base_of_v<concurrencpp::executor, executor_type>,
                      "concurrencpp::resume_on() - given executor does not derive from concurrencpp::executor");

        if (!static_cast<bool>(executor)) {
            throw std::invalid_argument(details::consts::k_resume_on_null_exception_err_msg);
        }

        return details::resume_on_awaitable<executor_type>(*executor);
    }

    template<class executor_type>
    auto resume_on(executor_type& executor) noexcept {
        return details::resume_on_awaitable<executor_type>(executor);
    }
}  // namespace concurrencpp

#endif
