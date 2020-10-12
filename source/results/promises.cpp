#include "promises.h"

using concurrencpp::details::coroutine_per_thread_data;

thread_local coroutine_per_thread_data
    coroutine_per_thread_data::s_tl_per_thread_data;

void concurrencpp::details::initial_accumulating_awaiter::await_suspend(
    std::experimental::coroutine_handle<> handle) const noexcept {
  auto& per_thread_data = coroutine_per_thread_data::s_tl_per_thread_data;
  auto accumulator = std::exchange(per_thread_data.accumulator, nullptr);

  assert(accumulator != nullptr);
  accumulator->push_back(handle);
}