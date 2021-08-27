#ifndef CONCURRENCPP_RESUME_ON_H
#define CONCURRENCPP_RESUME_ON_H

#include "concurrencpp/executors/executor.h"
#include "concurrencpp/results/impl/consumer_context.h"

namespace concurrencpp::details {
    template<class executor_type>
    class resume_on_awaitable : public suspend_always {

       private:
        await_context m_await_ctx;
        executor_type& m_executor;

       public:
        resume_on_awaitable(executor_type& executor) noexcept : m_executor(executor) {}

        resume_on_awaitable(const resume_on_awaitable&) = delete;
        resume_on_awaitable(resume_on_awaitable&&) = delete;

        resume_on_awaitable& operator=(const resume_on_awaitable&) = delete;
        resume_on_awaitable& operator=(resume_on_awaitable&&) = delete;

        void await_suspend(coroutine_handle<void> handle) {
            m_await_ctx.set_coro_handle(handle);
            m_executor.template post<await_via_functor>(&m_await_ctx);
        }

        void await_resume() const {
            m_await_ctx.throw_if_interrupted();
        }
    };
}  // namespace concurrencpp::details

namespace concurrencpp {
    template<class executor_type>
    auto resume_on(std::shared_ptr<executor_type> executor) {
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