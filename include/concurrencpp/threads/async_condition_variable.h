#ifndef CONCURRENCPP_ASYNC_CONDITION_VARIABLE_H
#define CONCURRENCPP_ASYNC_CONDITION_VARIABLE_H

#include "async_lock.h"

#include "concurrencpp/results/lazy_result.h"
#include "concurrencpp/coroutines/coroutine.h"

#include <mutex>

namespace concurrencpp {
    class async_condition_variable {

       private:
        class cv_awaitable {
           private:
            async_condition_variable& m_parent;
            scoped_async_lock& m_lock;
            const std::shared_ptr<executor> m_resume_executor;
            details::coroutine_handle<void> m_caller_handle;

           public:
            cv_awaitable* next = nullptr;

            cv_awaitable(async_condition_variable& parent,
                         scoped_async_lock& lock,
                         const std::shared_ptr<executor>& resume_executor) noexcept :
                m_parent(parent),
                m_lock(lock), m_resume_executor(resume_executor) {}

            void resume() noexcept {
                assert(static_cast<bool>(m_caller_handle));
                assert(!m_caller_handle.done());
                m_resume_executor->post(m_caller_handle);
            }

            bool await_ready() const noexcept {
                return false;
            }

            void await_suspend(details::coroutine_handle<void> caller_handle) {
                m_caller_handle = caller_handle;

                std::unique_lock<std::mutex> lock(m_parent.m_lock);
                m_lock.unlock();

                m_parent.m_awaiters.push_back(this);
            }

            void await_resume() const noexcept {}
        };

        class slist {

           private:
            cv_awaitable* m_head = nullptr;
            cv_awaitable* m_tail = nullptr;

           public:
            slist() noexcept = default;

            slist(slist&& rhs) noexcept : m_head(std::exchange(rhs.m_head, nullptr)), m_tail(std::exchange(rhs.m_tail, nullptr)) {}

            bool empty() const noexcept {
                return m_head == nullptr;
            }

            void push_back(cv_awaitable* node) noexcept {
                assert(node->next == nullptr);

                if (m_head == nullptr) {
                    assert(m_tail == nullptr);
                    m_head = node;
                    m_tail = node;
                    return;
                }

                assert(m_tail != nullptr);
                m_tail->next = node;
                m_tail = node;
            }

            cv_awaitable* pop_front() noexcept {
                if (m_head == nullptr) {
                    assert(m_tail == nullptr);
                    return nullptr;
                }

                assert(m_tail != nullptr);
                const auto node = m_head;
                m_head = m_head->next;

                if (m_head == nullptr) {
                    m_tail = nullptr;
                }

                return node;
            }
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