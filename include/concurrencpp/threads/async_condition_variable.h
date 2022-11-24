#ifndef CONCURRENCPP_ASYNC_CONDITION_VARIABLE_H
#define CONCURRENCPP_ASYNC_CONDITION_VARIABLE_H

#include "async_lock.h"

#include "concurrencpp/results/lazy_result.h"
#include "concurrencpp/coroutines/coroutine.h"

#include <mutex>

namespace concurrencpp {
    class CRCPP_API async_condition_variable {

       private:
        class CRCPP_API cv_awaitable {
           private:
            async_condition_variable& m_parent;
            scoped_async_lock& m_lock;
            const std::shared_ptr<executor> m_resume_executor;
            details::coroutine_handle<void> m_caller_handle;
            bool m_interrupted = false;

           public:
            cv_awaitable* next = nullptr;

            cv_awaitable(async_condition_variable& parent,
                         scoped_async_lock& lock,
                         const std::shared_ptr<executor>& resume_executor) noexcept;
            
            constexpr bool await_ready() const noexcept {
                return false;
            }
            
            void await_suspend(details::coroutine_handle<void> caller_handle);
            void await_resume() const;
            void resume() noexcept;
        };

        class CRCPP_API slist {

           private:
            cv_awaitable* m_head = nullptr;
            cv_awaitable* m_tail = nullptr;

           public:
            slist() noexcept = default;
            slist(slist&& rhs) noexcept;

            bool empty() const noexcept;

            void push_back(cv_awaitable* node) noexcept;
            cv_awaitable* pop_front() noexcept;
        };

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
        slist m_awaiters;

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
                std::is_convertible_v<std::invoke_result_t<predicate_type>, bool>,
                "concurrencpp::async_condition_variable::await - given predicate does not return a type which is or convertible to bool.");

            verify_await_params(resume_executor, lock);
            return await_impl(std::move(resume_executor), lock, pred);
        }

        void notify_one();
        void notify_all();
    };
}  // namespace concurrencpp

#endif