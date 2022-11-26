#ifndef CONCURRENCPP_ASYNC_CONDITION_VARIABLE_H
#define CONCURRENCPP_ASYNC_CONDITION_VARIABLE_H

#include "concurrencpp/threads/async_lock.h"
#include "concurrencpp/results/lazy_result.h"
#include "concurrencpp/coroutines/coroutine.h"

namespace concurrencpp::details {
    class CRCPP_API cv_awaitable;
}

namespace concurrencpp {
    class CRCPP_API async_condition_variable {

        friend details::cv_awaitable;

       private:
        class CRCPP_API slist {

           private:
            details::cv_awaitable* m_head = nullptr;
            details::cv_awaitable* m_tail = nullptr;

           public:
            slist() noexcept = default;
            slist(slist&& rhs) noexcept;

            bool empty() const noexcept;

            void push_back(details::cv_awaitable* node) noexcept;
            details::cv_awaitable* pop_front() noexcept;
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