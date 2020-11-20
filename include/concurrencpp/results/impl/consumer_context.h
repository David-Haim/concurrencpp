#ifndef CONCURRENCPP_CONSUMER_CONTEXT_H
#define CONCURRENCPP_CONSUMER_CONTEXT_H

#include "concurrencpp/task.h"
#include "concurrencpp/forward_declerations.h"

#include <mutex>
#include <condition_variable>
#include <experimental/coroutine>

namespace concurrencpp::details {
    class await_context {

       private:
        std::experimental::coroutine_handle<> m_handle;
        std::exception_ptr m_interrupt_exception;

       public:
        void set_coro_handle(std::experimental::coroutine_handle<> coro_handle) noexcept;
        void set_interrupt(const std::exception_ptr& interrupt);

        void operator()() noexcept;

        void throw_if_interrupted() const;

        concurrencpp::task to_task() noexcept;
    };

    class await_via_context {

       private:
        await_context m_await_context;
        std::shared_ptr<executor> m_executor;

       public:
        await_via_context() noexcept = default;
        await_via_context(std::shared_ptr<executor> executor) noexcept;

        void set_coro_handle(std::experimental::coroutine_handle<> coro_handle) noexcept;

        void operator()() noexcept;

        void throw_if_interrupted() const;
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
        when_any_context(std::shared_ptr<when_any_state_base> when_any_state, size_t index) noexcept;

        void operator()() const noexcept;
    };

    class consumer_context {

       private:
        enum class consumer_status { idle, await, await_via, wait, when_all, when_any };

        union storage {
            int idle;
            await_context* await_context;
            await_via_context* await_via_ctx;
            std::shared_ptr<wait_context> wait_ctx;
            std::shared_ptr<when_all_state_base> when_all_state;
            when_any_context when_any_ctx;

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

        void set_await_context(await_context* await_context) noexcept;

        void set_await_via_context(await_via_context* await_ctx) noexcept;

        void set_wait_context(std::shared_ptr<wait_context> wait_ctx) noexcept;

        void set_when_all_context(std::shared_ptr<when_all_state_base> when_all_state) noexcept;

        void set_when_any_context(std::shared_ptr<when_any_state_base> when_any_ctx, size_t index) noexcept;

        void operator()() noexcept;
    };
}  // namespace concurrencpp::details

#endif
