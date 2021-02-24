#include "concurrencpp/results/promises.h"
#include "concurrencpp/coroutines/coroutine.h"

using concurrencpp::details::coroutine_per_thread_data;

thread_local coroutine_per_thread_data coroutine_per_thread_data::s_tl_per_thread_data;

void concurrencpp::details::initial_accumulating_awaiter::await_suspend(coroutine_handle<void> handle) noexcept {
    m_await_via_context.set_coro_handle(handle);

    auto& per_thread_data = coroutine_per_thread_data::s_tl_per_thread_data;
    auto accumulator = std::exchange(per_thread_data.accumulator, nullptr);

    assert(accumulator != nullptr);
    assert(accumulator->capacity() > accumulator->size());  // so it's always noexcept
    accumulator->emplace_back(m_await_via_context.get_functor());
}

void concurrencpp::details::initial_accumulating_awaiter::await_resume() {
    m_await_via_context.throw_if_interrupted();
}
