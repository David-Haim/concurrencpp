#ifndef CONCURRENCPP_CONSUMER_CONTEXT_H
#define CONCURRENCPP_CONSUMER_CONTEXT_H

#include "concurrencpp/coroutines/coroutine.h"
#include "concurrencpp/results/result_fwd_declarations.h"

#include <mutex>
#include <condition_variable>

namespace concurrencpp::details {
    class await_via_functor {

       private:
        coroutine_handle<void> m_caller_handle;
        bool* m_interrupted;

       public:
        await_via_functor(coroutine_handle<void> caller_handle, bool* interrupted) noexcept;
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
        void wait();
        bool wait_for(size_t milliseconds);

        void notify();
    };

    class when_any_context {

       private:
        std::atomic<const result_state_base*> m_status;
        coroutine_handle<void> m_coro_handle;

        static const result_state_base* k_processing;
        static const result_state_base* k_done_processing;

       public:
        when_any_context(coroutine_handle<void> coro_handle) noexcept;

        bool any_result_finished() const noexcept;
        bool finish_processing() noexcept;
        const result_state_base* completed_result() const noexcept;

        void try_resume(result_state_base& completed_result) noexcept;
        bool resume_inline(result_state_base& completed_result) noexcept;
    };

    class consumer_context {

       private:
        enum class consumer_status { idle, await, wait_for, when_any };

        union storage {
            coroutine_handle<void> caller_handle;
            std::shared_ptr<wait_context> wait_for_ctx;
            std::shared_ptr<when_any_context> when_any_ctx;

            template<class type, class... argument_type>
            static void build(type& o, argument_type&&... arguments) noexcept {
                new (std::addressof(o)) type(std::forward<argument_type>(arguments)...);
            }

            template<class type>
            static void destroy(type& o) noexcept {
                o.~type();
            }

            storage() noexcept {}
            ~storage() noexcept {}
        };

       private:
        consumer_status m_status = consumer_status::idle;
        storage m_storage;

        void destroy() noexcept;

       public:
        ~consumer_context() noexcept;

        void clear() noexcept;
        void resume_consumer(result_state_base& self) const;

        void set_await_handle(coroutine_handle<void> caller_handle) noexcept;
        void set_wait_for_context(const std::shared_ptr<wait_context>& wait_ctx) noexcept;
        void set_when_any_context(const std::shared_ptr<when_any_context>& when_any_ctx) noexcept;
    };
}  // namespace concurrencpp::details

#endif
