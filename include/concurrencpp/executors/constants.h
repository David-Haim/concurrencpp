#ifndef CONCURRENCPP_EXECUTORS_CONSTS_H
#define CONCURRENCPP_EXECUTORS_CONSTS_H

#include <limits>
#include <numeric>

namespace concurrencpp::details::consts {
    inline const char* k_inline_executor_name = "inline_executor";
    constexpr int k_inline_executor_max_concurrency_level = 0;

    inline const char* k_thread_executor_name = "thread_executor";
    constexpr int k_thread_executor_max_concurrency_level = std::numeric_limits<int>::max();

    inline const char* k_thread_pool_executor_name = "thread_pool_executor";
    inline const char* k_background_executor_name = "background_executor";

    constexpr int k_worker_thread_max_concurrency_level = 1;
    inline const char* k_worker_thread_executor_name = "worker_thread_executor";

    inline const char* k_manual_executor_name = "manual_executor";
    constexpr int k_manual_executor_max_concurrency_level = std::numeric_limits<int>::max();

    inline const char* k_timer_queue_name = "timer_queue";

    inline const char* k_executor_shutdown_err_msg = " - shutdown has been called on this executor.";
}  // namespace concurrencpp::details::consts

#endif
