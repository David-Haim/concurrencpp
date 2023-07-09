#ifndef CONCURRENCPP_CONSTANTS_H
#define CONCURRENCPP_CONSTANTS_H

#include <cstddef>

namespace concurrencpp::details::consts {
    constexpr static size_t k_cpu_threadpool_worker_count_factor = 1;
    constexpr static size_t k_background_threadpool_worker_count_factor = 4;
    constexpr static size_t k_max_threadpool_worker_waiting_time_sec = 2 * 60;
    constexpr static size_t k_default_number_of_cores = 8;
    constexpr static size_t k_max_timer_queue_worker_waiting_time_sec = 2 * 60;

    constexpr static unsigned int k_concurrencpp_version_major = 0;
    constexpr static unsigned int k_concurrencpp_version_minor = 1;
    constexpr static unsigned int k_concurrencpp_version_revision = 7;
}  // namespace concurrencpp::details::consts

#endif
