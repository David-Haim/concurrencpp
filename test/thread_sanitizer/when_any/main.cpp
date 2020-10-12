#include "concurrencpp.h"

#include <iostream>
#include <random>

concurrencpp::result<void> when_any_vector_test(
    std::shared_ptr<concurrencpp::thread_executor> te);
concurrencpp::result<void> when_any_tuple_test(
    std::shared_ptr<concurrencpp::thread_executor> te);

int main() {
  concurrencpp::runtime runtime;
  when_any_vector_test(runtime.thread_executor()).get();
  std::cout << "=========================================" << std::endl;
  when_any_tuple_test(runtime.thread_executor()).get();
  std::cout << "=========================================" << std::endl;
  return 0;
}

using namespace concurrencpp;

struct random_ctx {
  std::random_device rd;
  std::mt19937 mt;
  std::uniform_int_distribution<size_t> dist;

  random_ctx() : mt(rd()), dist(1, 5 * 1000) {}

  size_t operator()() noexcept {
    return dist(mt);
  }
};

std::vector<result<int>> run_loop_once(std::shared_ptr<thread_executor> te,
                                       random_ctx& r,
                                       size_t count) {
  std::vector<result<int>> results;
  results.reserve(count);

  for (size_t i = 0; i < count; i++) {
    const auto sleeping_time = r();
    results.emplace_back(te->submit([sleeping_time] {
      std::this_thread::sleep_for(std::chrono::milliseconds(sleeping_time));
      return 0;
    }));
  }

  return results;
}

result<void> when_any_vector_test(
    std::shared_ptr<concurrencpp::thread_executor> te) {
  random_ctx r;

  auto loop = run_loop_once(te, r, 1'024);

  while (!loop.empty()) {
    auto any = co_await when_any(loop.begin(), loop.end());
    auto& done_result = any.results[any.index];

    if (done_result.status() != result_status::value) {
      std::abort();
    }

    if (done_result.get() != 0) {
      std::abort();
    }

    any.results.erase(any.results.begin() + any.index);
    loop = std::move(any.results);
  }
}

template<class... result_sequence>
std::vector<result<int>> to_vector(std::tuple<result_sequence...>& results) {
  return std::apply(
      [](auto&&... elems) {
        std::vector<result<int>> result;
        result.reserve(sizeof...(elems));
        (result.emplace_back(std::move(elems)), ...);
        return result;
      },
      std::forward<decltype(results)>(results));
}

concurrencpp::result<void> when_any_tuple_test(
    std::shared_ptr<concurrencpp::thread_executor> te) {
  random_ctx r;
  auto loop = run_loop_once(te, r, 10);

  for (size_t i = 0; i < 256; i++) {
    auto any = co_await when_any(std::move(loop[0]), std::move(loop[1]),
                                 std::move(loop[2]), std::move(loop[3]),
                                 std::move(loop[4]), std::move(loop[5]),
                                 std::move(loop[6]), std::move(loop[7]),
                                 std::move(loop[8]), std::move(loop[9]));

    loop = to_vector(any.results);

    const auto done_index = any.index;
    auto& done_result = loop[done_index];

    if (done_result.status() != result_status::value) {
      std::abort();
    }

    if (done_result.get() != 0) {
      std::abort();
    }

    loop.erase(loop.begin() + done_index);

    const auto sleeping_time = r();
    loop.emplace_back(te->submit([sleeping_time] {
      std::this_thread::sleep_for(std::chrono::milliseconds(sleeping_time));
      return 0;
    }));
  }
}
