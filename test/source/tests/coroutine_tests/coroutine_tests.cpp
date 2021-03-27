#include "concurrencpp/concurrencpp.h"

#include "infra/tester.h"
#include "infra/assertions.h"
#include "utils/object_observer.h"
#include "utils/test_ready_result.h"
#include "utils/executor_shutdowner.h"

#include <string>

using concurrencpp::result;

namespace concurrencpp::tests {
    template<class type>
    result<type> recursive_coroutine(executor_tag,
                                     std::shared_ptr<thread_executor> ex,
                                     const size_t cur_depth,
                                     const size_t max_depth,
                                     const bool terminate_by_exception);

    template<class type>
    void test_recursive_coroutines_impl();
    void test_recursive_coroutines();

    result<void> test_combo_coroutine_impl(std::shared_ptr<thread_executor> ex, const bool terminate_by_exception);
    void test_combo_coroutine();
}  // namespace concurrencpp::tests

template<class type>
result<type> concurrencpp::tests::recursive_coroutine(executor_tag,
                                                      std::shared_ptr<thread_executor> ex,
                                                      const size_t cur_depth,
                                                      const size_t max_depth,
                                                      const bool terminate_by_exception) {
    if (cur_depth < max_depth) {
        co_return co_await recursive_coroutine<type>({}, ex, cur_depth + 1, max_depth, terminate_by_exception);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    if (terminate_by_exception) {
        throw custom_exception(1234);
    }

    co_return value_gen<type>::default_value();
}

template<class type>
void concurrencpp::tests::test_recursive_coroutines_impl() {
    // value
    {
        auto ex = std::make_shared<concurrencpp::thread_executor>();
        executor_shutdowner es(ex);
        auto result = recursive_coroutine<type>({}, ex, 0, 20, false);
        result.wait();
        test_ready_result(std::move(result));
    }

    // exception
    {
        auto ex = std::make_shared<concurrencpp::thread_executor>();
        executor_shutdowner es(ex);
        auto result = recursive_coroutine<type>({}, ex, 0, 20, true);
        result.wait();
        test_ready_result_custom_exception(std::move(result), 1234);
    }
}

void concurrencpp::tests::test_recursive_coroutines() {
    test_recursive_coroutines_impl<int>();
    test_recursive_coroutines_impl<std::string>();
    test_recursive_coroutines_impl<void>();
    test_recursive_coroutines_impl<int&>();
    test_recursive_coroutines_impl<std::string&>();
}

result<void> concurrencpp::tests::test_combo_coroutine_impl(std::shared_ptr<thread_executor> ex, const bool terminate_by_exception) {
    auto int_result = co_await ex->submit([] {
        return value_gen<int>::default_value();
    });

    assert_equal(int_result, value_gen<int>::default_value());

    auto string_result = co_await ex->submit([int_result] {
        return std::to_string(int_result);
    });

    assert_equal(string_result, std::to_string(value_gen<int>::default_value()));

    co_await ex->submit([] {
    });

    auto& int_ref_result = co_await ex->submit([]() -> int& {
        return value_gen<int&>::default_value();
    });

    assert_equal(&int_ref_result, &value_gen<int&>::default_value());

    auto& str_ref_result = co_await ex->submit([]() -> std::string& {
        return value_gen<std::string&>::default_value();
    });

    assert_equal(&str_ref_result, &value_gen<std::string&>::default_value());

    if (terminate_by_exception) {
        throw custom_exception(1234);
    }
}

void concurrencpp::tests::test_combo_coroutine() {
    // value
    {
        auto te = std::make_shared<concurrencpp::thread_executor>();
        executor_shutdowner es(te);
        auto res = test_combo_coroutine_impl(te, false);
        res.wait();
        test_ready_result(std::move(res));
    }

    // exception
    {
        auto te = std::make_shared<concurrencpp::thread_executor>();
        executor_shutdowner es(te);
        auto res = test_combo_coroutine_impl(te, true);
        res.wait();
        test_ready_result_custom_exception(std::move(res), 1234);
    }
}

using namespace concurrencpp::tests;

int main() {
    tester tester("coroutines test");

    tester.add_step("recursive coroutines", test_recursive_coroutines);
    tester.add_step("combo coroutine", test_combo_coroutine);

    tester.launch_test();
    return 0;
}