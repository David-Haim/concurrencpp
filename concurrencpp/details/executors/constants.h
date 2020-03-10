#ifndef CONCURRENCPP_EXECUTORS_CONSTS_H
#define CONCURRENCPP_EXECUTORS_CONSTS_H

namespace concurrencpp::details::consts {
	inline const char* k_inline_executor_name = "concurrencpp::inline_executor";
	inline const char* k_thread_pool_executor_name = "concurrencpp::thread_pool_executor";
	inline const char* k_background_executor_name = "concurrencpp::background_executor";
	inline const char* k_thread_executor_name = "concurrencpp::thread_executor";
	inline const char* k_worker_thread_executor_name = "concurrencpp::worker_thread_executor";
	inline const char* k_manual_executor_name = "concurrencpp::manual_executor";

	inline const char* k_thread_pool_executor_cancel_error_msg = 
		"concurrencpp::thread_pool_executor::~thread_pool_executor() was called.";

	inline const char* k_background_executor_cancel_error_msg =
		"concurrencpp::background_executor::~background_executor() was called.";
	
	inline const char* k_thread_executor_cancel_error_msg =
		"concurrencpp::thread_executor::~thread_executor() was called.";

	inline const char* k_worker_thread_executor_cancel_error_msg =
		"concurrencpp::worker_thread_executor::~worker_thread_executor() was called.";

	inline const char* k_manual_executor_cancel_error_msg = 
		"concurrencpp::manual_executor::~manual_executor() was called.";
}

#endif