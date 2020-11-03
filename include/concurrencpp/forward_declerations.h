#ifndef CONCURRENCPP_FORWARD_DECLERATIONS_H
#define CONCURRENCPP_FORWARD_DECLERATIONS_H

namespace concurrencpp {
    struct null_result;

    template<class type>
    class result;
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
}  // namespace concurrencpp

#endif  // FORWARD_DECLERATIONS_H
