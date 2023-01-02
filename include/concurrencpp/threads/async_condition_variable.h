#ifndef CONCURRENCPP_ASYNC_CONDITION_VARIABLE_H
#define CONCURRENCPP_ASYNC_CONDITION_VARIABLE_H

#include "concurrencpp/utils/slist.h"
#include "concurrencpp/threads/async_lock.h"
#include "concurrencpp/results/lazy_result.h"
#include "concurrencpp/coroutines/coroutine.h"
#include "concurrencpp/forward_declarations.h"

namespace concurrencpp::details {
    class CRCPP_API cv_awaiter {
       private:
        async_condition_variable& m_parent;
        scoped_async_lock& m_lock;
        coroutine_handle<void> m_caller_handle;

       public:
        cv_awaiter* next = nullptr;

        cv_awaiter(async_condition_variable& parent, scoped_async_lock& lock) noexcept;

        constexpr bool await_ready() const noexcept {
            return false;
        }

        void await_suspend(details::coroutine_handle<void> caller_handle);
        void await_resume() const noexcept {}
        void resume() noexcept;
    };
}  // namespace concurrencpp::details

namespace concurrencpp {
    class CRCPP_API async_condition_variable {

        friend details::cv_awaiter;

       private:
        template<class predicate_type>
        lazy_result<void> await_impl(std::shared_ptr<executor> resume_executor, scoped_async_lock& lock, predicate_type pred) {
            while (true) {
                assert(lock.owns_lock());
                if (pred()) {
                    break;
                }

                co_await await_impl(resume_executor, lock);
            }
        }

       private:
        std::mutex m_lock;
        details::slist<details::cv_awaiter> m_awaiters;

        static void verify_await_params(const std::shared_ptr<executor>& resume_executor, const scoped_async_lock& lock);

        lazy_result<void> await_impl(std::shared_ptr<executor> resume_executor, scoped_async_lock& lock);

       public:
        async_condition_variable() noexcept = default;
        ~async_condition_variable() noexcept;

        async_condition_variable(const async_condition_variable&) noexcept = delete;
        async_condition_variable(async_condition_variable&&) noexcept = delete;

        lazy_result<void> await(std::shared_ptr<executor> resume_executor, scoped_async_lock& lock);

        template<class predicate_type>
        lazy_result<void> await(std::shared_ptr<executor> resume_executor, scoped_async_lock& lock, predicate_type pred) {
            static_assert(
                std::is_invocable_r_v<bool, predicate_type>,
                "concurrencpp::async_condition_variable::await - given predicate isn't invocable with no arguments, or does not return a type which is or convertible to bool.");

            verify_await_params(resume_executor, lock);
            return await_impl(std::move(resume_executor), lock, pred);
        }

        void notify_one();
        void notify_all();
    };
}  // namespace concurrencpp

#endif