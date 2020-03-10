#include "runtime.h"
#include "constants.h"

#include "../threads/thread_pool.h"
#include "../threads/thread_group.h"
#include "../threads/constants.h"
#include "../executors/constants.h"

namespace concurrencpp::details {
	size_t hardware_concurrency() noexcept {
		const auto hc = std::thread::hardware_concurrency();
		return (hc != 0) ? hc : consts::k_default_number_of_cores;
	}

	size_t default_max_cpu_workers() noexcept {
		return static_cast<size_t>(hardware_concurrency() * consts::k_cpu_threadpool_worker_count_factor);
	}

	size_t default_max_background_workers() noexcept {
		return static_cast<size_t>(hardware_concurrency() * consts::k_background_threadpool_worker_count_factor);
	}

	constexpr static std::chrono::seconds k_default_max_worker_wait_time = 
		std::chrono::seconds(consts::k_max_worker_waiting_time_sec);

	void interrupt_thread() { throw thread_interrupter(); }
}

using concurrencpp::details::thread_group;
using concurrencpp::details::thread_pool;

using concurrencpp::runtime;
using concurrencpp::runtime_options;

using concurrencpp::inline_executor;
using concurrencpp::thread_pool_executor;
using concurrencpp::parallel_executor;
using concurrencpp::background_executor;
using concurrencpp::thread_executor;
using concurrencpp::worker_thread_executor;
using concurrencpp::manual_executor;

/*
	runtime_options
*/

runtime_options::runtime_options() noexcept :
	max_cpu_threads(details::default_max_cpu_workers()),
	max_cpu_thread_waiting_time(details::k_default_max_worker_wait_time),
	max_background_threads(details::default_max_background_workers()),
	max_background_thread_waiting_time(details::k_default_max_worker_wait_time){}

/*
	runtime
*/

runtime::runtime(const runtime::context& ctx) : 
	runtime(ctx, runtime_options()){}

runtime::runtime(const runtime::context& ctx, const runtime_options& options){
	m_timer_queue = std::make_shared<::concurrencpp::timer_queue>();

	m_inline_executor = std::make_shared<::concurrencpp::inline_executor>(inline_executor::context());

	thread_pool_executor::context thread_pool_executor_ctx;
	thread_pool_executor_ctx.cancellation_msg = details::consts::k_thread_pool_executor_cancel_error_msg;
	thread_pool_executor_ctx.max_waiting_time = options.max_cpu_thread_waiting_time;
	thread_pool_executor_ctx.max_worker_count = options.max_cpu_threads;

	m_thread_pool_executor = std::make_shared<::concurrencpp::thread_pool_executor>(thread_pool_executor_ctx);
	
	background_executor::context background_executor_ctx;
	background_executor_ctx.cancellation_msg = details::consts::k_background_executor_cancel_error_msg;
	background_executor_ctx.max_waiting_time = options.max_background_thread_waiting_time;
	background_executor_ctx.max_worker_count = options.max_background_threads;

	m_background_executor = std::make_shared<::concurrencpp::background_executor>(background_executor_ctx);
	
	m_thread_executor = std::make_shared<::concurrencpp::thread_executor>(thread_executor::context());
}

runtime::~runtime() noexcept {}

std::shared_ptr<concurrencpp::timer_queue> concurrencpp::runtime::timer_queue() const noexcept {
	return m_timer_queue;
}

std::shared_ptr<concurrencpp::inline_executor> concurrencpp::runtime::inline_executor() noexcept {
	return m_inline_executor;
}

std::shared_ptr<concurrencpp::thread_pool_executor> concurrencpp::runtime::thread_pool_executor() noexcept {
	return m_thread_pool_executor;
}

std::shared_ptr<concurrencpp::background_executor> concurrencpp::runtime::background_executor() noexcept {
	return m_background_executor;
}

std::shared_ptr<concurrencpp::thread_executor> concurrencpp::runtime::thread_executor()  noexcept {
	return m_thread_executor;
}

std::shared_ptr<concurrencpp::worker_thread_executor> concurrencpp::runtime::worker_thread_executor() const {
	return std::make_shared<concurrencpp::worker_thread_executor>(worker_thread_executor::context{});
}

std::shared_ptr<concurrencpp::manual_executor> concurrencpp::runtime::manual_executor() const {
	return std::make_shared<concurrencpp::manual_executor>(manual_executor::context{});
}

std::tuple<unsigned int, unsigned int, unsigned int> concurrencpp::runtime::version() noexcept{
	return {
		details::consts::k_concurrencpp_version_major,
		details::consts::k_concurrencpp_version_minor,
		details::consts::k_concurrencpp_version_revision
	};
}

std::shared_ptr<runtime> concurrencpp::make_runtime() {
	return std::make_shared<concurrencpp::runtime>(runtime::context{});
}

std::shared_ptr<runtime> concurrencpp::make_runtime(const runtime_options& opts) {
	return std::make_shared<concurrencpp::runtime>(runtime::context{}, opts);
}