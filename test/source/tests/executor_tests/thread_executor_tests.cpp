#include "concurrencpp/concurrencpp.h"

#include "infra/tester.h"
#include "infra/assertions.h"
#include "utils/object_observer.h"
#include "utils/test_generators.h"
#include "utils/executor_shutdowner.h"

namespace concurrencpp::tests {
    void test_thread_executor_name();

    void test_thread_executor_shutdown_join();
    void test_thread_executor_shutdown_method_access();
    void test_thread_executor_shutdown_more_than_once();
    void test_thread_executor_shutdown();

    void test_thread_executor_max_concurrency_level();

    void test_thread_executor_post_foreign();
    void test_thread_executor_post_inline();
    void test_thread_executor_post();

    void test_thread_executor_submit_foreign();
    void test_thread_executor_submit_inline();
    void test_thread_executor_submit();

    void test_thread_executor_bulk_post_foreign();
    void test_thread_executor_bulk_post_inline();
    void test_thread_executor_bulk_post();

    void test_thread_executor_bulk_submit_foreign();
    void test_thread_executor_bulk_submit_inline();
    void test_thread_executor_bulk_submit();

    void assert_unique_execution_threads(const std::unordered_map<size_t, size_t>& execution_map, const size_t expected_thread_count) {
        assert_equal(execution_map.size(), expected_thread_count);

        for (const auto& [thread_id, invocation_count] : execution_map) {
            assert_not_equal(thread_id, static_cast<size_t>(0));
            assert_equal(invocation_count, static_cast<size_t>(1));
        }
    }
}  // namespace concurrencpp::tests

void concurrencpp::tests::test_thread_executor_name() {
    auto executor = std::make_shared<concurrencpp::thread_executor>();
    executor_shutdowner shutdown(executor);

    assert_equal(executor->name, concurrencpp::details::consts::k_thread_executor_name);
}

void concurrencpp::tests::test_thread_executor_shutdown_method_access() {
    auto executor = std::make_shared<thread_executor>();
    assert_false(executor->shutdown_requested());

    executor->shutdown();
    assert_true(executor->shutdown_requested());

    assert_throws<concurrencpp::errors::runtime_shutdown>([executor] {
        executor->enqueue(concurrencpp::task {});
    });

    assert_throws<concurrencpp::errors::runtime_shutdown>([executor] {
        concurrencpp::task array[4];
        std::span<concurrencpp::task> span = array;
        executor->enqueue(span);
    });
}

void concurrencpp::tests::test_thread_executor_shutdown_join() {
    // the executor returns only when all tasks are done.
    auto executor = std::make_shared<thread_executor>();
    object_observer observer;
    const size_t task_count = 16;

    for (size_t i = 0; i < task_count; i++) {
        executor->post([stub = observer.get_testing_stub()]() mutable {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            stub();
        });
    }

    executor->shutdown();
    assert_true(executor->shutdown_requested());

    assert_equal(observer.get_execution_count(), task_count);
    assert_equal(observer.get_destruction_count(), task_count);
}

void concurrencpp::tests::test_thread_executor_shutdown_more_than_once() {
    auto executor = std::make_shared<thread_executor>();
    for (size_t i = 0; i < 4; i++) {
        executor->shutdown();
    }
}

void concurrencpp::tests::test_thread_executor_shutdown() {
    test_thread_executor_shutdown_method_access();
    test_thread_executor_shutdown_join();
    test_thread_executor_shutdown_more_than_once();
}

void concurrencpp::tests::test_thread_executor_max_concurrency_level() {
    auto executor = std::make_shared<thread_executor>();
    executor_shutdowner shutdown(executor);

    assert_equal(executor->max_concurrency_level(), concurrencpp::details::consts::k_thread_executor_max_concurrency_level);
}

void concurrencpp::tests::test_thread_executor_post_foreign() {
    object_observer observer;
    const size_t task_count = 128;
    auto executor = std::make_shared<thread_executor>();
    executor_shutdowner shutdown(executor);

    for (size_t i = 0; i < task_count; i++) {
        executor->post(observer.get_testing_stub());
    }

    assert_true(observer.wait_execution_count(task_count, std::chrono::minutes(1)));
    assert_true(observer.wait_destruction_count(task_count, std::chrono::minutes(1)));

    assert_unique_execution_threads(observer.get_execution_map(), task_count);
}

void concurrencpp::tests::test_thread_executor_post_inline() {
    object_observer observer;
    constexpr size_t task_count = 128;
    auto executor = std::make_shared<thread_executor>();
    executor_shutdowner shutdown(executor);

    executor->post([executor, &observer] {
        for (size_t i = 0; i < task_count; i++) {
            executor->post(observer.get_testing_stub());
        }
    });

    assert_true(observer.wait_execution_count(task_count, std::chrono::minutes(1)));
    assert_true(observer.wait_destruction_count(task_count, std::chrono::minutes(1)));

    assert_unique_execution_threads(observer.get_execution_map(), task_count);
}

void concurrencpp::tests::test_thread_executor_post() {
    test_thread_executor_post_foreign();
    test_thread_executor_post_inline();
}

void concurrencpp::tests::test_thread_executor_submit_foreign() {
    object_observer observer;
    const size_t task_count = 128;
    auto executor = std::make_shared<thread_executor>();
    executor_shutdowner shutdown(executor);

    std::vector<result<size_t>> results;
    results.resize(task_count);

    for (size_t i = 0; i < task_count; i++) {
        results[i] = executor->submit(observer.get_testing_stub(i));
    }

    for (size_t i = 0; i < task_count; i++) {
        assert_equal(results[i].get(), i);
    }

    assert_true(observer.wait_execution_count(task_count, std::chrono::minutes(1)));
    assert_true(observer.wait_destruction_count(task_count, std::chrono::minutes(1)));

    assert_unique_execution_threads(observer.get_execution_map(), task_count);
}

void concurrencpp::tests::test_thread_executor_submit_inline() {
    object_observer observer;
    constexpr size_t task_count = 128;
    auto executor = std::make_shared<thread_executor>();
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
        assert_equal(results[i].get(), i);
    }

    assert_true(observer.wait_execution_count(task_count, std::chrono::minutes(1)));
    assert_true(observer.wait_destruction_count(task_count, std::chrono::minutes(1)));

    assert_unique_execution_threads(observer.get_execution_map(), task_count);
}

void concurrencpp::tests::test_thread_executor_submit() {
    test_thread_executor_submit_foreign();
    test_thread_executor_submit_inline();
}

void concurrencpp::tests::test_thread_executor_bulk_post_foreign() {
    object_observer observer;
    const size_t task_count = 128;
    auto executor = std::make_shared<thread_executor>();
    executor_shutdowner shutdown(executor);

    std::vector<testing_stub> stubs;
    stubs.reserve(task_count);

    for (size_t i = 0; i < task_count; i++) {
        stubs.emplace_back(observer.get_testing_stub());
    }

    executor->bulk_post<testing_stub>(stubs);

    assert_true(observer.wait_execution_count(task_count, std::chrono::minutes(1)));
    assert_true(observer.wait_destruction_count(task_count, std::chrono::minutes(1)));

    assert_unique_execution_threads(observer.get_execution_map(), task_count);
}

void concurrencpp::tests::test_thread_executor_bulk_post_inline() {
    object_observer observer;
    constexpr size_t task_count = 128;
    auto executor = std::make_shared<thread_executor>();
    executor_shutdowner shutdown(executor);

    executor->post([executor, &observer]() mutable {
        std::vector<testing_stub> stubs;
        stubs.reserve(task_count);

        for (size_t i = 0; i < task_count; i++) {
            stubs.emplace_back(observer.get_testing_stub());
        }

        executor->bulk_post<testing_stub>(stubs);
    });

    assert_true(observer.wait_execution_count(task_count, std::chrono::minutes(1)));
    assert_true(observer.wait_destruction_count(task_count, std::chrono::minutes(1)));

    assert_unique_execution_threads(observer.get_execution_map(), task_count);
}

void concurrencpp::tests::test_thread_executor_bulk_post() {
    test_thread_executor_bulk_post_foreign();
    test_thread_executor_bulk_post_inline();
}

void concurrencpp::tests::test_thread_executor_bulk_submit_foreign() {
    object_observer observer;
    const size_t task_count = 128;
    auto executor = std::make_shared<thread_executor>();
    executor_shutdowner shutdown(executor);

    std::vector<value_testing_stub> stubs;
    stubs.reserve(task_count);

    for (size_t i = 0; i < task_count; i++) {
        stubs.emplace_back(observer.get_testing_stub(i));
    }

    auto results = executor->bulk_submit<value_testing_stub>(stubs);

    for (size_t i = 0; i < task_count; i++) {
        assert_equal(results[i].get(), i);
    }

    assert_true(observer.wait_execution_count(task_count, std::chrono::minutes(1)));
    assert_true(observer.wait_destruction_count(task_count, std::chrono::minutes(1)));

    assert_unique_execution_threads(observer.get_execution_map(), task_count);
}

void concurrencpp::tests::test_thread_executor_bulk_submit_inline() {
    object_observer observer;
    constexpr size_t task_count = 128;
    auto executor = std::make_shared<thread_executor>();
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
        assert_equal(results[i].get(), i);
    }

    assert_true(observer.wait_execution_count(task_count, std::chrono::minutes(1)));
    assert_true(observer.wait_destruction_count(task_count, std::chrono::minutes(1)));

    assert_unique_execution_threads(observer.get_execution_map(), task_count);
}

void concurrencpp::tests::test_thread_executor_bulk_submit() {
    test_thread_executor_bulk_post_foreign();
    test_thread_executor_bulk_post_inline();
}

using namespace concurrencpp::tests;

int main() {
    tester tester("thread_executor test");

    tester.add_step("shutdown", test_thread_executor_shutdown);
    tester.add_step("name", test_thread_executor_name);
    tester.add_step("max_concurrency_level", test_thread_executor_max_concurrency_level);
    tester.add_step("post", test_thread_executor_post);
    tester.add_step("submit", test_thread_executor_submit);
    tester.add_step("bulk_post", test_thread_executor_bulk_post);
    tester.add_step("bulk_submit", test_thread_executor_bulk_submit);

    tester.launch_test();
    return 0;
}