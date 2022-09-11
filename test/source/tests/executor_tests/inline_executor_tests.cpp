#include "concurrencpp/concurrencpp.h"

#include "infra/tester.h"
#include "infra/assertions.h"
#include "utils/object_observer.h"
#include "utils/test_generators.h"
#include "utils/test_ready_result.h"
#include "utils/executor_shutdowner.h"

namespace concurrencpp::tests {
    void test_inline_executor_name();

    void test_inline_executor_shutdown();

    void test_inline_executor_max_concurrency_level();

    void test_inline_executor_post_exception();
    void test_inline_executor_post_foreign();
    void test_inline_executor_post_inline();
    void test_inline_executor_post();

    void test_inline_executor_submit_exception();
    void test_inline_executor_submit_foreign();
    void test_inline_executor_submit_inline();
    void test_inline_executor_submit();

    void test_inline_executor_bulk_post_exception();
    void test_inline_executor_bulk_post_foreign();
    void test_inline_executor_bulk_post_inline();
    void test_inline_executor_bulk_post();

    void test_inline_executor_bulk_submit_exception();
    void test_inline_executor_bulk_submit_foreign();
    void test_inline_executor_bulk_submit_inline();
    void test_inline_executor_bulk_submit();

    void assert_executed_inline(const std::unordered_map<size_t, size_t>& execution_map) noexcept {
        assert_equal(execution_map.size(), static_cast<size_t>(1));
        assert_equal(execution_map.begin()->first, ::concurrencpp::details::thread::get_current_virtual_id());
    }
}  // namespace concurrencpp::tests

using concurrencpp::details::thread;

void concurrencpp::tests::test_inline_executor_name() {
    auto executor = std::make_shared<inline_executor>();
    executor_shutdowner shutdown(executor);

    assert_equal(executor->name, concurrencpp::details::consts::k_inline_executor_name);
}

void concurrencpp::tests::test_inline_executor_shutdown() {
    auto executor = std::make_shared<inline_executor>();
    assert_false(executor->shutdown_requested());

    executor->shutdown();
    assert_true(executor->shutdown_requested());

    // it's ok to shut down an executor more than once
    executor->shutdown();

    assert_throws<concurrencpp::errors::runtime_shutdown>([executor] {
        executor->enqueue(concurrencpp::task {});
    });

    assert_throws<concurrencpp::errors::runtime_shutdown>([executor] {
        concurrencpp::task array[4];
        std::span<concurrencpp::task> span = array;
        executor->enqueue(span);
    });
}

void concurrencpp::tests::test_inline_executor_max_concurrency_level() {
    auto executor = std::make_shared<inline_executor>();
    executor_shutdowner shutdown(executor);

    assert_equal(executor->max_concurrency_level(), concurrencpp::details::consts::k_inline_executor_max_concurrency_level);
}

void concurrencpp::tests::test_inline_executor_post_exception() {
    auto executor = std::make_shared<inline_executor>();
    executor_shutdowner shutdown(executor);

    executor->post([] {
        throw std::runtime_error("");
    });
}

void concurrencpp::tests::test_inline_executor_post_foreign() {
    object_observer observer;
    const size_t task_count = 1'024;
    auto executor = std::make_shared<inline_executor>();
    executor_shutdowner shutdown(executor);

    for (size_t i = 0; i < task_count; i++) {
        executor->post(observer.get_testing_stub());
    }

    assert_equal(observer.get_execution_count(), task_count);
    assert_equal(observer.get_destruction_count(), task_count);
    assert_executed_inline(observer.get_execution_map());
}

void concurrencpp::tests::test_inline_executor_post_inline() {
    object_observer observer;
    constexpr size_t task_count = 1'024;
    auto executor = std::make_shared<inline_executor>();
    executor_shutdowner shutdown(executor);

    executor->post([executor, &observer] {
        for (size_t i = 0; i < task_count; i++) {
            executor->post(observer.get_testing_stub());
        }
    });

    assert_equal(observer.get_execution_count(), task_count);
    assert_equal(observer.get_destruction_count(), task_count);
    assert_executed_inline(observer.get_execution_map());
}

void concurrencpp::tests::test_inline_executor_post() {
    test_inline_executor_post_exception();
    test_inline_executor_post_inline();
    test_inline_executor_post_foreign();
}

void concurrencpp::tests::test_inline_executor_submit_exception() {
    auto executor = std::make_shared<inline_executor>();
    executor_shutdowner shutdown(executor);
    constexpr intptr_t id = 12345;

    auto result = executor->submit([id] {
        throw custom_exception(id);
    });

    result.wait();
    test_ready_result_custom_exception(std::move(result), id);
}

void concurrencpp::tests::test_inline_executor_submit_foreign() {
    object_observer observer;
    const size_t task_count = 1'024;
    auto executor = std::make_shared<inline_executor>();
    executor_shutdowner shutdown(executor);

    std::vector<result<size_t>> results;
    results.resize(task_count);

    for (size_t i = 0; i < task_count; i++) {
        results[i] = executor->submit(observer.get_testing_stub(i));
    }

    for (size_t i = 0; i < task_count; i++) {
        assert_equal(results[i].status(), result_status::value);
        assert_equal(results[i].get(), i);
    }

    assert_equal(observer.get_execution_count(), task_count);
    assert_equal(observer.get_destruction_count(), task_count);
    assert_executed_inline(observer.get_execution_map());
}

void concurrencpp::tests::test_inline_executor_submit_inline() {
    object_observer observer;
    constexpr size_t task_count = 1'024;
    auto executor = std::make_shared<inline_executor>();
    executor_shutdowner shutdown(executor);

    auto results_res = executor->submit([executor, &observer] {
        std::vector<result<size_t>> results;
        results.resize(task_count);

        for (size_t i = 0; i < task_count; i++) {
            results[i] = executor->submit(observer.get_testing_stub(i));
        }

        return results;
    });

    auto results = results_res.get();
    for (size_t i = 0; i < task_count; i++) {
        assert_equal(results[i].status(), result_status::value);
        assert_equal(results[i].get(), i);
    }

    assert_equal(observer.get_execution_count(), task_count);
    assert_equal(observer.get_destruction_count(), task_count);
    assert_executed_inline(observer.get_execution_map());
}

void concurrencpp::tests::test_inline_executor_submit() {
    test_inline_executor_submit_exception();
    test_inline_executor_submit_foreign();
    test_inline_executor_submit_inline();
}

void concurrencpp::tests::test_inline_executor_bulk_post_exception() {
    auto executor = std::make_shared<inline_executor>();
    executor_shutdowner shutdown(executor);

    auto thrower = [] {
        throw std::runtime_error("");
    };

    std::vector<decltype(thrower)> tasks;
    tasks.resize(4);

    executor->bulk_post<decltype(thrower)>(tasks);
}

void concurrencpp::tests::test_inline_executor_bulk_post_foreign() {
    object_observer observer;
    const size_t task_count = 1'024;
    auto executor = std::make_shared<inline_executor>();
    executor_shutdowner shutdown(executor);

    std::vector<testing_stub> stubs;
    stubs.reserve(task_count);

    for (size_t i = 0; i < task_count; i++) {
        stubs.emplace_back(observer.get_testing_stub());
    }

    std::span<testing_stub> span = stubs;
    executor->bulk_post<testing_stub>(span);

    assert_equal(observer.get_execution_count(), task_count);
    assert_equal(observer.get_destruction_count(), task_count);
    assert_executed_inline(observer.get_execution_map());
}

void concurrencpp::tests::test_inline_executor_bulk_post_inline() {
    object_observer observer;
    constexpr size_t task_count = 1'024;
    auto executor = std::make_shared<inline_executor>();
    executor_shutdowner shutdown(executor);

    executor->post([executor, &observer]() mutable {
        std::vector<testing_stub> stubs;
        stubs.reserve(task_count);

        for (size_t i = 0; i < task_count; i++) {
            stubs.emplace_back(observer.get_testing_stub());
        }

        executor->bulk_post<testing_stub>(stubs);
    });

    assert_equal(observer.get_execution_count(), task_count);
    assert_equal(observer.get_destruction_count(), task_count);
    assert_executed_inline(observer.get_execution_map());
}

void concurrencpp::tests::test_inline_executor_bulk_post() {
    test_inline_executor_bulk_post_exception();
    test_inline_executor_bulk_post_foreign();
    test_inline_executor_bulk_post_inline();
}

void concurrencpp::tests::test_inline_executor_bulk_submit_exception() {
    auto executor = std::make_shared<inline_executor>();
    executor_shutdowner shutdown(executor);
    constexpr intptr_t id = 12345;

    auto thrower = [] {
        throw custom_exception(id);
    };

    std::vector<decltype(thrower)> tasks;
    tasks.resize(4, thrower);

    auto results = executor->bulk_submit<decltype(thrower)>(tasks);

    for (auto& result : results) {
        result.wait();
        test_ready_result_custom_exception(std::move(result), id);
    }
}

void concurrencpp::tests::test_inline_executor_bulk_submit_foreign() {
    object_observer observer;
    const size_t task_count = 1'024;
    auto executor = std::make_shared<inline_executor>();
    executor_shutdowner shutdown(executor);

    std::vector<value_testing_stub> stubs;
    stubs.reserve(task_count);

    for (size_t i = 0; i < task_count; i++) {
        stubs.emplace_back(observer.get_testing_stub(i));
    }

    auto results = executor->bulk_submit<value_testing_stub>(stubs);
    for (size_t i = 0; i < task_count; i++) {
        assert_equal(results[i].status(), result_status::value);
        assert_equal(results[i].get(), i);
    }

    assert_equal(observer.get_execution_count(), task_count);
    assert_equal(observer.get_destruction_count(), task_count);
    assert_executed_inline(observer.get_execution_map());
}

void concurrencpp::tests::test_inline_executor_bulk_submit_inline() {
    object_observer observer;
    constexpr size_t task_count = 1'024;
    auto executor = std::make_shared<inline_executor>();
    executor_shutdowner shutdown(executor);

    auto results_res = executor->submit([executor, &observer] {
        std::vector<value_testing_stub> stubs;
        stubs.reserve(task_count);

        for (size_t i = 0; i < task_count; i++) {
            stubs.emplace_back(observer.get_testing_stub(i));
        }

        return executor->bulk_submit<value_testing_stub>(stubs);
    });

    auto results = results_res.get();
    for (size_t i = 0; i < task_count; i++) {
        assert_equal(results[i].status(), result_status::value);
        assert_equal(results[i].get(), i);
    }

    assert_equal(observer.get_execution_count(), task_count);
    assert_equal(observer.get_destruction_count(), task_count);
    assert_executed_inline(observer.get_execution_map());
}

void concurrencpp::tests::test_inline_executor_bulk_submit() {
    test_inline_executor_bulk_submit_exception();
    test_inline_executor_bulk_submit_foreign();
    test_inline_executor_bulk_submit_inline();
}

using namespace concurrencpp::tests;

int main() {
    tester tester("inline_executor test");

    tester.add_step("name", test_inline_executor_name);
    tester.add_step("shutdown", test_inline_executor_shutdown);
    tester.add_step("max_concurrency_level", test_inline_executor_max_concurrency_level);
    tester.add_step("post", test_inline_executor_post);
    tester.add_step("submit", test_inline_executor_submit);
    tester.add_step("bulk_post", test_inline_executor_bulk_post);
    tester.add_step("bulk_submit", test_inline_executor_bulk_submit);

    tester.launch_test();
    return 0;
}