#ifndef CONCURRENCPP_FORWARD_DECLARATIONS_H
#define CONCURRENCPP_FORWARD_DECLARATIONS_H

namespace concurrencpp {
    struct null_result;

    template<class type>
    class result;

    template<class type>
    class lazy_result;

    template<class type>
    class shared_result;

    template<class type>
    class result_promise;

    class runtime;

    class timer_queue;
    class timer;

    class executor;
    class inline_executor;
    class thread_pool_executor;
    class thread_executor;
    class worker_thread_executor;
    class manual_executor;

    template<typename type>
    class generator;

    class async_lock;
    class async_condition_variable;
}  // namespace concurrencpp

#endif  // FORWARD_DECLARATIONS_H
