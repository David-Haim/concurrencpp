#include "concurrencpp/results/promises.h"
#include "concurrencpp/coroutines/coroutine.h"

using concurrencpp::details::coroutine_per_thread_data;

thread_local coroutine_per_thread_data coroutine_per_thread_data::s_tl_per_thread_data;

void concurrencpp::details::initial_accumulating_awaiter::await_suspend(coroutine_handle<void> handle) noexcept {
    auto& per_thread_data = coroutine_per_thread_data::s_tl_per_thread_data;
    auto accumulator = std::exchange(per_thread_data.accumulator, nullptr);

    assert(accumulator != nullptr);
    assert(accumulator->capacity() > accumulator->size());  // so it's always noexcept
    accumulator->emplace_back(await_via_functor {handle, &m_interrupted});
}

void concurrencpp::details::initial_accumulating_awaiter::await_resume() const {
    if (m_interrupted) {
        throw errors::broken_task(consts::k_broken_task_exception_error_msg);
    }
}
