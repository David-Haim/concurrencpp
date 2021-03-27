#ifndef CONCURRENCPP_CONSUMER_CONTEXT_H
#define CONCURRENCPP_CONSUMER_CONTEXT_H

#include "concurrencpp/coroutines/coroutine.h"
#include "concurrencpp/results/result_fwd_declerations.h"

#include <mutex>
#include <condition_variable>

namespace concurrencpp::details {
    class await_via_functor;

    class await_via_context {

       public:
        class await_context {

           private:
            coroutine_handle<void> handle;
            std::exception_ptr interrupt_exception;

           public:
            void resume() noexcept;

            void set_coro_handle(coroutine_handle<void> coro_handle) noexcept;
            void set_interrupt(const std::exception_ptr& interrupt) noexcept;

            void throw_if_interrupted() const;
        };

       private:
        await_context m_await_ctx;
        std::shared_ptr<executor> m_executor;

       public:
        await_via_context() noexcept = default;
        await_via_context(const std::shared_ptr<executor>& executor) noexcept;

        void operator()() noexcept;

        void resume() noexcept;

        void set_coro_handle(coroutine_handle<void> coro_handle) noexcept;
        void set_interrupt(const std::exception_ptr& interrupt) noexcept;

        void throw_if_interrupted() const;

        await_via_functor get_functor() noexcept;
    };

    class await_via_functor {

       private:
        await_via_context::await_context* m_ctx;

       public:
        await_via_functor(await_via_context::await_context* ctx) noexcept;
        await_via_functor(await_via_functor&& rhs) noexcept;
        ~await_via_functor() noexcept;

        void operator()() noexcept;
    };

    class wait_context {

       private:
        std::mutex m_lock;
        std::condition_variable m_condition;
        bool m_ready = false;

       public:
        void wait() noexcept;
        bool wait_for(size_t milliseconds) noexcept;

        void notify() noexcept;
    };

    class when_all_state_base {

       protected:
        std::atomic_size_t m_counter;
        std::recursive_mutex m_lock;

       public:
        virtual ~when_all_state_base() noexcept = default;
        virtual void on_result_ready() noexcept = 0;
    };

    class when_any_state_base {

       protected:
        std::atomic_bool m_fulfilled = false;
        std::recursive_mutex m_lock;

       public:
        virtual ~when_any_state_base() noexcept = default;
        virtual void on_result_ready(size_t) noexcept = 0;
    };

    class when_any_context {

       private:
        std::shared_ptr<when_any_state_base> m_when_any_state;
        size_t m_index;

       public:
        when_any_context(const std::shared_ptr<when_any_state_base>& when_any_state, size_t index) noexcept;
        when_any_context(const when_any_context&) noexcept = default;

        void operator()() const noexcept;
    };

    class consumer_context {

       private:
        enum class consumer_status { idle, await, await_via, wait, when_all, when_any, shared };

        union storage {
            int idle;
            coroutine_handle<void> caller_handle;
            await_via_context* await_via_ctx;
            std::shared_ptr<wait_context> wait_ctx;
            std::shared_ptr<when_all_state_base> when_all_ctx;
            when_any_context when_any_ctx;
            std::weak_ptr<shared_result_state_base> shared_ctx;

            template<class type, class... argument_type>
            static void build(type& o, argument_type&&... arguments) noexcept {
                new (std::addressof(o)) type(std::forward<argument_type>(arguments)...);
            }

            template<class type>
            static void destroy(type& o) noexcept {
                o.~type();
            }

            storage() noexcept : idle() {}
            ~storage() noexcept {}
        };

       private:
        consumer_status m_status;
        storage m_storage;

       public:
        consumer_context() noexcept;
        ~consumer_context() noexcept;

        void clear() noexcept;
        void resume_consumer() const noexcept;

        void set_await_handle(coroutine_handle<void> caller_handle) noexcept;
        void set_await_via_context(await_via_context& await_ctx) noexcept;
        void set_wait_context(const std::shared_ptr<wait_context>& wait_ctx) noexcept;
        void set_when_all_context(const std::shared_ptr<when_all_state_base>& when_all_state) noexcept;
        void set_when_any_context(const std::shared_ptr<when_any_state_base>& when_any_ctx, size_t index) noexcept;
        void set_shared_context(const std::weak_ptr<shared_result_state_base>& shared_result_state) noexcept;
    };
}  // namespace concurrencpp::details

#endif
