#include "concurrencpp/runtime/runtime.h"
#include "concurrencpp/runtime/constants.h"

#include "concurrencpp/executors/constants.h"
#include "concurrencpp/executors/inline_executor.h"
#include "concurrencpp/executors/thread_pool_executor.h"
#include "concurrencpp/executors/thread_executor.h"
#include "concurrencpp/executors/worker_thread_executor.h"
#include "concurrencpp/executors/manual_executor.h"

#include "concurrencpp/timers/timer_queue.h"

#include <algorithm>

namespace concurrencpp::details {
    namespace {
        size_t default_max_cpu_workers() noexcept {
            return static_cast<size_t>(thread::hardware_concurrency() * consts::k_cpu_threadpool_worker_count_factor);
        }

        size_t default_max_background_workers() noexcept {
            return static_cast<size_t>(thread::hardware_concurrency() * consts::k_background_threadpool_worker_count_factor);
        }

        constexpr auto k_default_max_worker_wait_time = std::chrono::seconds(consts::k_max_threadpool_worker_waiting_time_sec);
    }  // namespace
}  // namespace concurrencpp::details

using concurrencpp::runtime;
using concurrencpp::runtime_options;
using concurrencpp::details::executor_collection;

/*
        executor_collection;
*/

void executor_collection::register_executor(std::shared_ptr<executor> executor) {
    assert(static_cast<bool>(executor));

    std::unique_lock<decltype(m_lock)> lock(m_lock);
    assert(std::find(m_executors.begin(), m_executors.end(), executor) == m_executors.end());
    m_executors.emplace_back(std::move(executor));
}

void executor_collection::shutdown_all() {
    std::unique_lock<decltype(m_lock)> lock(m_lock);
    for (auto& executor : m_executors) {
        assert(static_cast<bool>(executor));
        executor->shutdown();
    }

    m_executors = {};
}

/*
        runtime_options
*/

runtime_options::runtime_options() noexcept :
    max_cpu_threads(details::default_max_cpu_workers()),
    max_thread_pool_executor_waiting_time(details::k_default_max_worker_wait_time),
    max_background_threads(details::default_max_background_workers()),
    max_background_executor_waiting_time(details::k_default_max_worker_wait_time),
    max_timer_queue_waiting_time(std::chrono::seconds(details::consts::k_max_timer_queue_worker_waiting_time_sec)) {}

/*
        runtime
*/

runtime::runtime() : runtime(runtime_options()) {}

runtime::runtime(const runtime_options& options) {
    m_timer_queue = std::make_shared<::concurrencpp::timer_queue>(options.max_timer_queue_waiting_time,
                                                                  options.thread_started_callback,
                                                                  options.thread_terminated_callback);

    m_inline_executor = std::make_shared<::concurrencpp::inline_executor>();
    m_registered_executors.register_executor(m_inline_executor);

    m_thread_pool_executor = std::make_shared<::concurrencpp::thread_pool_executor>(details::consts::k_thread_pool_executor_name,
                                                                                    options.max_cpu_threads,
                                                                                    options.max_thread_pool_executor_waiting_time,
                                                                                    options.thread_started_callback,
                                                                                    options.thread_terminated_callback);
    m_registered_executors.register_executor(m_thread_pool_executor);

    m_background_executor = std::make_shared<::concurrencpp::thread_pool_executor>(details::consts::k_background_executor_name,
                                                                                   options.max_background_threads,
                                                                                   options.max_background_executor_waiting_time,
                                                                                   options.thread_started_callback,
                                                                                   options.thread_terminated_callback);
    m_registered_executors.register_executor(m_background_executor);

    m_thread_executor =
        std::make_shared<::concurrencpp::thread_executor>(options.thread_started_callback, options.thread_terminated_callback);
    m_registered_executors.register_executor(m_thread_executor);
}

concurrencpp::runtime::~runtime() noexcept {
    try {
        m_timer_queue->shutdown();
        m_registered_executors.shutdown_all();

    } catch (...) {
        std::abort();
    }
}

std::shared_ptr<concurrencpp::timer_queue> runtime::timer_queue() const noexcept {
    return m_timer_queue;
}

std::shared_ptr<concurrencpp::inline_executor> runtime::inline_executor() const noexcept {
    return m_inline_executor;
}

std::shared_ptr<concurrencpp::thread_pool_executor> runtime::thread_pool_executor() const noexcept {
    return m_thread_pool_executor;
}

std::shared_ptr<concurrencpp::thread_pool_executor> runtime::background_executor() const noexcept {
    return m_background_executor;
}

std::shared_ptr<concurrencpp::thread_executor> runtime::thread_executor() const noexcept {
    return m_thread_executor;
}

std::shared_ptr<concurrencpp::worker_thread_executor> runtime::make_worker_thread_executor() {
    auto executor = std::make_shared<worker_thread_executor>();
    m_registered_executors.register_executor(executor);
    return executor;
}

std::shared_ptr<concurrencpp::manual_executor> runtime::make_manual_executor() {
    auto executor = std::make_shared<concurrencpp::manual_executor>();
    m_registered_executors.register_executor(executor);
    return executor;
}

std::tuple<unsigned int, unsigned int, unsigned int> runtime::version() noexcept {
    return {details::consts::k_concurrencpp_version_major,
            details::consts::k_concurrencpp_version_minor,
            details::consts::k_concurrencpp_version_revision};
}
