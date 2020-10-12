#include "concurrencpp/concurrencpp.h"
#include "tests/all_tests.h"

#include "tests/test_utils/test_ready_result.h"
#include "tests/test_utils/executor_shutdowner.h"

#include "tester/tester.h"
#include "helpers/assertions.h"
#include "helpers/random.h"
#include "helpers/object_observer.h"

namespace concurrencpp::tests {
template<class type>
void test_when_any_vector_empty_result();

template<class type>
void test_when_any_vector_empty_range();

template<class type>
result<void> test_when_any_vector_valid(std::shared_ptr<thread_executor> ex);

template<class type>
void test_when_any_vector_impl();

void test_when_any_vector();

void test_when_any_tuple_empty_result();

result<void> test_when_any_tuple_impl(std::shared_ptr<thread_executor> ex);

void test_when_any_tuple();
}  // namespace concurrencpp::tests

template<class type>
void concurrencpp::tests::test_when_any_vector_empty_result() {
  const size_t task_count = 63;
  std::vector<result_promise<type>> result_promises(task_count);
  std::vector<result<type>> results;

  for (auto& rp : result_promises) {
    results.emplace_back(rp.get_result());
  }

  results.emplace_back();

  assert_throws_with_error_message<errors::empty_result>(
      [&] {
        concurrencpp::when_any(results.begin(), results.end());
      },
      concurrencpp::details::consts::k_when_any_empty_result_error_msg);

  const auto all_valid = std::all_of(
      results.begin(), results.begin() + task_count, [](const auto& result) {
        return static_cast<bool>(result);
      });

  assert_true(all_valid);
}

template<class type>
void concurrencpp::tests::test_when_any_vector_empty_range() {
  std::vector<result<type>> empty_range;

  assert_throws_with_error_message<std::invalid_argument>(
      [&] {
        when_any(empty_range.begin(), empty_range.end());
      },
      concurrencpp::details::consts::k_when_any_empty_range_error_msg);
}

template<class type>
concurrencpp::result<void> concurrencpp::tests::test_when_any_vector_valid(
    std::shared_ptr<thread_executor> ex) {
  const size_t task_count = 64;
  auto values = result_factory<type>::get_many(task_count);
  std::vector<result<type>> results;
  random randomizer;

  for (size_t i = 0; i < task_count; i++) {
    const auto time_to_sleep = randomizer(10, 100);
    results.emplace_back(ex->submit([i, time_to_sleep, &values]() -> type {
      (void)values;
      std::this_thread::sleep_for(std::chrono::milliseconds(time_to_sleep));

      if (i % 4 == 0) {
        throw costume_exception(i);
      }

      if constexpr (!std::is_same_v<void, type>) {
        return values[i];
      }
    }));
  }

  auto any_done = co_await when_any(results.begin(), results.end());

  auto& done_result = any_done.results[any_done.index];

  const auto all_valid = std::all_of(
      any_done.results.begin(), any_done.results.end(), [](const auto& result) {
        return static_cast<bool>(result);
      });

  assert_true(all_valid);

  if (any_done.index % 4 == 0) {
    test_ready_result_costume_exception(std::move(done_result), any_done.index);
  } else {
    if constexpr (std::is_same_v<void, type>) {
      test_ready_result_result(std::move(done_result));
    } else {
      test_ready_result_result(std::move(done_result), values[any_done.index]);
    }
  }

  // the value vector is a local variable, tasks may outlive it. join them.
  for (auto& result : any_done.results) {
    if (!static_cast<bool>(result)) {
      continue;
    }

    co_await result.resolve();
  }
}

template<class type>
void concurrencpp::tests::test_when_any_vector_impl() {
  test_when_any_vector_empty_result<type>();
  test_when_any_vector_empty_range<type>();

  {
    auto thread_executor = std::make_shared<concurrencpp::thread_executor>();
    executor_shutdowner es(thread_executor);
    test_when_any_vector_valid<type>(thread_executor).get();
  }
}

void concurrencpp::tests::test_when_any_vector() {
  test_when_any_vector_impl<int>();
  test_when_any_vector_impl<std::string>();
  test_when_any_vector_impl<void>();
  test_when_any_vector_impl<int&>();
  test_when_any_vector_impl<std::string&>();
}

void concurrencpp::tests::test_when_any_tuple_empty_result() {
  result_promise<int> rp_int;
  auto int_res = rp_int.get_result();

  result_promise<std::string> rp_s;
  auto s_res = rp_s.get_result();

  result_promise<void> rp_void;
  auto void_res = rp_void.get_result();

  result_promise<int&> rp_int_ref;
  auto int_ref_res = rp_int_ref.get_result();

  result<std::string&> s_ref_res;

  assert_throws_with_error_message<errors::empty_result>(
      [&] {
        when_any(std::move(int_res), std::move(s_res), std::move(void_res),
                 std::move(int_ref_res), std::move(s_ref_res));
      },
      concurrencpp::details::consts::k_when_any_empty_result_error_msg);

  // all pre-operation results are still valid
  assert_true(static_cast<bool>(int_res));
  assert_true(static_cast<bool>(s_res));
  assert_true(static_cast<bool>(void_res));
  assert_true(static_cast<bool>(int_res));
}

concurrencpp::result<void> concurrencpp::tests::test_when_any_tuple_impl(
    std::shared_ptr<thread_executor> ex) {
  std::atomic_size_t counter = 0;
  random randomizer;

  auto tts = randomizer(10, 100);
  auto int_res_val = ex->submit([&counter, tts] {
    std::this_thread::sleep_for(std::chrono::milliseconds(tts));
    counter.fetch_add(1, std::memory_order_relaxed);
    return result_factory<int>::get();
  });

  tts = randomizer(10, 100);
  auto int_res_ex = ex->submit([&counter, tts] {
    std::this_thread::sleep_for(std::chrono::milliseconds(tts));
    counter.fetch_add(1, std::memory_order_relaxed);
    throw costume_exception(0);
    return result_factory<int>::get();
  });

  tts = randomizer(10, 100);
  auto s_res_val = ex->submit([&counter, tts]() -> std::string {
    std::this_thread::sleep_for(std::chrono::milliseconds(tts));
    counter.fetch_add(1, std::memory_order_relaxed);
    return result_factory<std::string>::get();
  });

  tts = randomizer(10, 100);
  auto s_res_ex = ex->submit([&counter, tts]() -> std::string {
    std::this_thread::sleep_for(std::chrono::milliseconds(tts));
    counter.fetch_add(1, std::memory_order_relaxed);
    throw costume_exception(1);
    return result_factory<std::string>::get();
  });

  tts = randomizer(10, 100);
  auto void_res_val = ex->submit([&counter, tts] {
    std::this_thread::sleep_for(std::chrono::milliseconds(tts));
    counter.fetch_add(1, std::memory_order_relaxed);
  });

  tts = randomizer(10, 100);
  auto void_res_ex = ex->submit([&counter, tts] {
    std::this_thread::sleep_for(std::chrono::milliseconds(tts));
    counter.fetch_add(1, std::memory_order_relaxed);
    throw costume_exception(2);
  });

  tts = randomizer(10, 100);
  auto int_ref_res_val = ex->submit([&counter, tts]() -> int& {
    std::this_thread::sleep_for(std::chrono::milliseconds(tts));
    counter.fetch_add(1, std::memory_order_relaxed);
    return result_factory<int&>::get();
  });

  tts = randomizer(10, 100);
  auto int_ref_res_ex = ex->submit([&counter, tts]() -> int& {
    std::this_thread::sleep_for(std::chrono::milliseconds(tts));
    counter.fetch_add(1, std::memory_order_relaxed);
    throw costume_exception(3);
    return result_factory<int&>::get();
  });

  tts = randomizer(10, 100);
  auto s_ref_res_val = ex->submit([&counter, tts]() -> std::string& {
    std::this_thread::sleep_for(std::chrono::milliseconds(tts));
    counter.fetch_add(1, std::memory_order_relaxed);
    return result_factory<std::string&>::get();
  });

  tts = randomizer(10, 100);
  auto s_ref_res_ex = ex->submit([&counter, tts]() -> std::string& {
    std::this_thread::sleep_for(std::chrono::milliseconds(tts));
    counter.fetch_add(1, std::memory_order_relaxed);
    throw costume_exception(4);
    return result_factory<std::string&>::get();
  });

  auto any_done = co_await when_any(
      std::move(int_res_val), std::move(int_res_ex), std::move(s_res_val),
      std::move(s_res_ex), std::move(void_res_val), std::move(void_res_ex),
      std::move(int_ref_res_val), std::move(int_ref_res_ex),
      std::move(s_ref_res_val), std::move(s_ref_res_ex));

  assert_bigger_equal(counter.load(std::memory_order_relaxed), size_t(1));

  switch (any_done.index) {
    case 0: {
      test_ready_result_result(std::move(std::get<0>(any_done.results)),
                               result_factory<int>::get());
      break;
    }
    case 1: {
      test_ready_result_costume_exception(
          std::move(std::get<1>(any_done.results)), 0);
      break;
    }
    case 2: {
      test_ready_result_result(std::move(std::get<2>(any_done.results)),
                               result_factory<std::string>::get());
      break;
    }
    case 3: {
      test_ready_result_costume_exception(
          std::move(std::get<3>(any_done.results)), 1);
      break;
    }
    case 4: {
      test_ready_result_result(std::move(std::get<4>(any_done.results)));
      break;
    }
    case 5: {
      test_ready_result_costume_exception(
          std::move(std::get<5>(any_done.results)), 2);
      break;
    }
    case 6: {
      test_ready_result_result(std::move(std::get<6>(any_done.results)),
                               result_factory<int&>::get());
      break;
    }
    case 7: {
      test_ready_result_costume_exception(
          std::move(std::get<7>(any_done.results)), 3);
      break;
    }
    case 8: {
      test_ready_result_result(std::move(std::get<8>(any_done.results)),
                               result_factory<std::string&>::get());
      break;
    }
    case 9: {
      test_ready_result_costume_exception(
          std::move(std::get<9>(any_done.results)), 4);
      break;
    }
    default: {
      assert_false(true);
    }
  }

  auto wait = [](auto& result) {
    if (static_cast<bool>(result)) {
      result.wait();
    }
  };

  std::apply(
      [wait](auto&... results) {
        (wait(results), ...);
      },
      any_done.results);
}

void concurrencpp::tests::test_when_any_tuple() {
  test_when_any_tuple_empty_result();

  {
    auto thread_executor = std::make_shared<concurrencpp::thread_executor>();
    executor_shutdowner es(thread_executor);
    test_when_any_tuple_impl(thread_executor).get();
  }
}

void concurrencpp::tests::test_when_any() {
  tester test("when_any test");

  test.add_step("when_any(begin, end)", test_when_any_vector);
  test.add_step("when_any(result_types&& ... results)", test_when_any_tuple);

  test.launch_test();
}
