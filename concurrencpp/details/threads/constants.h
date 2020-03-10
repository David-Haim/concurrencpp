#ifndef CONCURRENCPP_THREADS_CONSTS_H
#define CONCURRENCPP_THREADS_CONSTS_H

namespace concurrencpp::details::consts {
	constexpr static float k_cpu_threadpool_worker_count_factor = 1.25f;
	constexpr static size_t k_background_threadpool_worker_count_factor = 2;
	constexpr static size_t k_max_worker_waiting_time_sec = 2 * 60;
	constexpr static size_t k_default_number_of_cores = 8;
}

#endif