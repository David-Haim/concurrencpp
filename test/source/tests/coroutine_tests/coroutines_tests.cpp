#include "concurrencpp.h"

#include "../all_tests.h"

#include "../test_utils/test_ready_result.h"

#include "../../tester/tester.h"
#include "../../helpers/assertions.h"
#include "../../helpers/object_observer.h"

#include <string>

#include "../test_utils/executor_shutdowner.h"

namespace concurrencpp::tests {
template<class type>
result<type> recursive_coroutine(executor_tag,
                                 std::shared_ptr<thread_executor> te,
                                 const size_t cur_depth,
                                 const size_t max_depth,
                                 const bool terminate_by_exception);

template<class type>
void test_recursive_coroutines_impl();
void test_recursive_coroutines();

result<void> test_combo_coroutine_impl(std::shared_ptr<thread_executor> te,
                                       const bool terminate_by_exception);
void test_combo_coroutine();
}  // namespace concurrencpp::tests

using concurrencpp::result;

template<class type>
result<type> concurrencpp::tests::recursive_coroutine(
    executor_tag,
    std::shared_ptr<thread_executor> te,
    const size_t cur_depth,
    const size_t max_depth,
    const bool terminate_by_exception) {

  if (cur_depth < max_depth) {
    co_return co_await recursive_coroutine<type>(
        {}, te, cur_depth + 1, max_depth, terminate_by_exception);
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(20));

  if (terminate_by_exception) {
    throw costume_exception(1234);
  }

  co_return result_factory<type>::get();
}

template<class type>
void concurrencpp::tests::test_recursive_coroutines_impl() {
  // value
  {
    auto te = std::make_shared<concurrencpp::thread_executor>();
    executor_shutdowner es(te);
    auto result = recursive_coroutine<type>({}, te, 0, 20, false);
    result.wait();
    test_ready_result_result(std::move(result));
  }

  // exception
  {
    auto te = std::make_shared<concurrencpp::thread_executor>();
    executor_shutdowner es(te);
    auto result = recursive_coroutine<type>({}, te, 0, 20, true);
    result.wait();
    test_ready_result_costume_exception(std::move(result), 1234);
  }
}

void concurrencpp::tests::test_recursive_coroutines() {
  test_recursive_coroutines_impl<int>();
  test_recursive_coroutines_impl<std::string>();
  test_recursive_coroutines_impl<void>();
  test_recursive_coroutines_impl<int&>();
  test_recursive_coroutines_impl<std::string&>();
}

result<void> concurrencpp::tests::test_combo_coroutine_impl(
    std::shared_ptr<thread_executor> te,
    const bool terminate_by_exception) {
  auto int_result = co_await te->submit([] {
    return result_factory<int>::get();
  });

  auto string_result = co_await te->submit([int_result] {
    return std::to_string(int_result);
  });

  assert_equal(string_result, std::to_string(result_factory<int>::get()));

  auto& int_ref_result = co_await te->submit([]() -> int& {
    return result_factory<int&>::get();
  });

  auto& str_ref_result = co_await te->submit([]() -> std::string& {
    return result_factory<std::string&>::get();
  });

  assert_equal(&int_ref_result, &result_factory<int&>::get());
  assert_equal(&str_ref_result, &result_factory<std::string&>::get());

  if (terminate_by_exception) {
    throw costume_exception(1234);
  }
}

void concurrencpp::tests::test_combo_coroutine() {
  // value
  {
    auto te = std::make_shared<concurrencpp::thread_executor>();
    executor_shutdowner es(te);
    auto res = test_combo_coroutine_impl(te, false);
    res.wait();
    test_ready_result_result(std::move(res));
  }

  // exception
  {
    auto te = std::make_shared<concurrencpp::thread_executor>();
    executor_shutdowner es(te);
    auto res = test_combo_coroutine_impl(te, true);
    res.wait();
    test_ready_result_costume_exception(std::move(res), 1234);
  }
}

void concurrencpp::tests::test_coroutines() {
  tester tester("coroutines test");

  tester.add_step("recursive coroutines", test_recursive_coroutines);
  tester.add_step("combo coroutine", test_combo_coroutine);

  tester.launch_test();
}