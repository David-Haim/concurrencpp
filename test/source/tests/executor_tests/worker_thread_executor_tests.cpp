#include "concurrencpp/concurrencpp.h"

#include "tests/all_tests.h"

#include "tests/test_utils/executor_shutdowner.h"

#include "tester/tester.h"
#include "helpers/assertions.h"
#include "helpers/object_observer.h"

#include "concurrencpp/executors/constants.h"

namespace concurrencpp::tests {
    void test_worker_thread_executor_name();

    void test_worker_thread_executor_shutdown_method_access();
    void test_worker_thread_executor_shutdown_thread_join();
    void test_worker_thread_executor_shutdown_coro_more_than_once();
    void test_worker_thread_executor_shutdown();

    void test_worker_thread_executor_max_concurrency_level();

    void test_worker_thread_executor_post_foreign();
    void test_worker_thread_executor_post_inline();
    void test_worker_thread_executor_post();

    void test_worker_thread_executor_submit_foreign();
    void test_worker_thread_executor_submit_inline();
    void test_worker_thread_executor_submit();

    void test_worker_thread_executor_bulk_post_foreign();
    void test_worker_thread_executor_bulk_post_inline();
    void test_worker_thread_executor_bulk_post();

    void test_worker_thread_executor_bulk_submit_foreign();
    void test_worker_thread_executor_bulk_submit_inline();
    void test_worker_thread_executor_bulk_submit();
}  // namespace concurrencpp::tests

using concurrencpp::details::thread;

void concurrencpp::tests::test_worker_thread_executor_name() {
    auto executor = std::make_shared<worker_thread_executor>();
    executor_shutdowner shutdown(executor);

    assert_equal(executor->name, concurrencpp::details::consts::k_worker_thread_executor_name);
}

void concurrencpp::tests::test_worker_thread_executor_shutdown_method_access() {
    auto executor = std::make_shared<worker_thread_executor>();
    assert_false(executor->shutdown_requested());

    executor->shutdown();
    assert_true(executor->shutdown_requested());

    assert_throws<concurrencpp::errors::executor_shutdown>([executor] {
        executor->enqueue(concurrencpp::task {});
    });

    assert_throws<concurrencpp::errors::executor_shutdown>([executor] {
        concurrencpp::task array[4];
        std::span<concurrencpp::task> span = array;
        executor->enqueue(span);
    });
}

void concurrencpp::tests::test_worker_thread_executor_shutdown_thread_join() {
    auto executor = std::make_shared<worker_thread_executor>();

    executor->post([] {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    });

    executor->shutdown();
    assert_true(executor->shutdown_requested());
}

void concurrencpp::tests::test_worker_thread_executor_shutdown_coro_more_than_once() {
    const size_t task_count = 64;
    auto executor = std::make_shared<worker_thread_executor>();

    for (size_t i = 0; i < task_count; i++) {
        executor->post([] {
            std::this_thread::sleep_for(std::chrono::milliseconds(18));
        });
    }

    for (size_t i = 0; i < 4; i++) {
        executor->shutdown();
    }
}

void concurrencpp::tests::test_worker_thread_executor_shutdown() {
    test_worker_thread_executor_shutdown_method_access();
    test_worker_thread_executor_shutdown_thread_join();
    test_worker_thread_executor_shutdown_coro_more_than_once();
}

void concurrencpp::tests::test_worker_thread_executor_max_concurrency_level() {
    auto executor = std::make_shared<worker_thread_executor>();
    executor_shutdowner shutdown(executor);

    assert_equal(executor->max_concurrency_level(), 1);
}

void concurrencpp::tests::test_worker_thread_executor_post_foreign() {
    object_observer observer;
    const size_t task_count = 1'024;
    auto executor = std::make_shared<worker_thread_executor>();
    executor_shutdowner shutdown(executor);

    for (size_t i = 0; i < task_count; i++) {
        executor->post(observer.get_testing_stub());
    }

    assert_true(observer.wait_execution_count(task_count, std::chrono::minutes(1)));
    assert_true(observer.wait_destruction_count(task_count, std::chrono::minutes(1)));

    const auto& execution_map = observer.get_execution_map();

    assert_equal(execution_map.size(), size_t(1));
    assert_not_equal(execution_map.begin()->first, thread::get_current_virtual_id());
}

void concurrencpp::tests::test_worker_thread_executor_post_inline() {
    object_observer observer;
    constexpr size_t task_count = 1'024;
    auto executor = std::make_shared<worker_thread_executor>();
    executor_shutdowner shutdown(executor);

    executor->post([executor, &observer] {
        for (size_t i = 0; i < task_count; i++) {
            executor->post(observer.get_testing_stub());
        }
    });

    assert_true(observer.wait_execution_count(task_count, std::chrono::minutes(1)));
    assert_true(observer.wait_destruction_count(task_count, std::chrono::minutes(1)));

    const auto& execution_map = observer.get_execution_map();

    assert_equal(execution_map.size(), size_t(1));
    assert_not_equal(execution_map.begin()->first, thread::get_current_virtual_id());
}

void concurrencpp::tests::test_worker_thread_executor_post() {
    test_worker_thread_executor_post_foreign();
    test_worker_thread_executor_post_inline();
}

void concurrencpp::tests::test_worker_thread_executor_submit_foreign() {
    object_observer observer;
    const size_t task_count = 1'024;
    auto executor = std::make_shared<worker_thread_executor>();
    executor_shutdowner shutdown(executor);

    std::vector<result<size_t>> results;
    results.resize(task_count);

    for (size_t i = 0; i < task_count; i++) {
        results[i] = executor->submit(observer.get_testing_stub(i));
    }

    assert_true(observer.wait_execution_count(task_count, std::chrono::minutes(1)));
    assert_true(observer.wait_destruction_count(task_count, std::chrono::minutes(1)));

    const auto& execution_map = observer.get_execution_map();

    assert_equal(execution_map.size(), size_t(1));
    assert_not_equal(execution_map.begin()->first, thread::get_current_virtual_id());

    for (size_t i = 0; i < task_count; i++) {
        assert_equal(results[i].get(), i);
    }
}

void concurrencpp::tests::test_worker_thread_executor_submit_inline() {
    object_observer observer;
    constexpr size_t task_count = 1'024;
    auto executor = std::make_shared<worker_thread_executor>();
    executor_shutdowner shutdown(executor);

    auto results_res = executor->submit([executor, &observer] {
        std::vector<result<size_t>> results;
        results.resize(task_count);
        for (size_t i = 0; i < task_count; i++) {
            results[i] = executor->submit(observer.get_testing_stub(i));
        }

        return results;
    });

    assert_true(observer.wait_execution_count(task_count, std::chrono::minutes(1)));
    assert_true(observer.wait_destruction_count(task_count, std::chrono::minutes(1)));

    const auto& execution_map = observer.get_execution_map();

    assert_equal(execution_map.size(), size_t(1));
    assert_not_equal(execution_map.begin()->first, thread::get_current_virtual_id());

    auto results = results_res.get();
    for (size_t i = 0; i < task_count; i++) {
        assert_equal(results[i].get(), size_t(i));
    }
}

void concurrencpp::tests::test_worker_thread_executor_submit() {
    test_worker_thread_executor_submit_foreign();
    test_worker_thread_executor_submit_inline();
}

void concurrencpp::tests::test_worker_thread_executor_bulk_post_foreign() {
    object_observer observer;
    const size_t task_count = 1'024;
    auto executor = std::make_shared<worker_thread_executor>();
    executor_shutdowner shutdown(executor);

    std::vector<testing_stub> stubs;
    stubs.reserve(task_count);

    for (size_t i = 0; i < task_count; i++) {
        stubs.emplace_back(observer.get_testing_stub());
    }

    executor->bulk_post<testing_stub>(stubs);

    assert_true(observer.wait_execution_count(task_count, std::chrono::minutes(1)));
    assert_true(observer.wait_destruction_count(task_count, std::chrono::minutes(1)));

    const auto& execution_map = observer.get_execution_map();

    assert_equal(execution_map.size(), size_t(1));
    assert_not_equal(execution_map.begin()->first, thread::get_current_virtual_id());
}

void concurrencpp::tests::test_worker_thread_executor_bulk_post_inline() {
    object_observer observer;
    constexpr size_t task_count = 1'024;
    auto executor = std::make_shared<worker_thread_executor>();
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

    const auto& execution_map = observer.get_execution_map();

    assert_equal(execution_map.size(), size_t(1));
    assert_not_equal(execution_map.begin()->first, thread::get_current_virtual_id());
}

void concurrencpp::tests::test_worker_thread_executor_bulk_post() {
    test_worker_thread_executor_bulk_post_foreign();
    test_worker_thread_executor_bulk_post_inline();
}

void concurrencpp::tests::test_worker_thread_executor_bulk_submit_foreign() {
    object_observer observer;
    const size_t task_count = 1'024;
    auto executor = std::make_shared<worker_thread_executor>();
    executor_shutdowner shutdown(executor);

    std::vector<value_testing_stub> stubs;
    stubs.reserve(task_count);

    for (size_t i = 0; i < task_count; i++) {
        stubs.emplace_back(observer.get_testing_stub(i));
    }

    auto results = executor->bulk_submit<value_testing_stub>(stubs);

    assert_true(observer.wait_execution_count(task_count, std::chrono::minutes(1)));
    assert_true(observer.wait_destruction_count(task_count, std::chrono::minutes(1)));

    const auto& execution_map = observer.get_execution_map();

    assert_equal(execution_map.size(), size_t(1));
    assert_not_equal(execution_map.begin()->first, thread::get_current_virtual_id());

    for (size_t i = 0; i < task_count; i++) {
        assert_equal(results[i].get(), i);
    }
}

void concurrencpp::tests::test_worker_thread_executor_bulk_submit_inline() {
    object_observer observer;
    constexpr size_t task_count = 1'024;
    auto executor = std::make_shared<worker_thread_executor>();
    executor_shutdowner shutdown(executor);

    auto results_res = executor->submit([executor, &observer] {
        std::vector<value_testing_stub> stubs;
        stubs.reserve(task_count);

        for (size_t i = 0; i < task_count; i++) {
            stubs.emplace_back(observer.get_testing_stub(i));
        }

        return executor->bulk_submit<value_testing_stub>(stubs);
    });

    assert_true(observer.wait_execution_count(task_count, std::chrono::minutes(1)));
    assert_true(observer.wait_destruction_count(task_count, std::chrono::minutes(1)));

    const auto& execution_map = observer.get_execution_map();

    assert_equal(execution_map.size(), size_t(1));
    assert_not_equal(execution_map.begin()->first, thread::get_current_virtual_id());

    auto results = results_res.get();
    for (size_t i = 0; i < task_count; i++) {
        assert_equal(results[i].get(), i);
    }
}

void concurrencpp::tests::test_worker_thread_executor_bulk_submit() {
    test_worker_thread_executor_bulk_submit_foreign();
    test_worker_thread_executor_bulk_submit_inline();
}

void concurrencpp::tests::test_worker_thread_executor() {
    tester tester("worker_thread_executor test");

    tester.add_step("name", test_worker_thread_executor_name);
    tester.add_step("shutdown", test_worker_thread_executor_shutdown);
    tester.add_step("max_concurrency_level", test_worker_thread_executor_max_concurrency_level);
    tester.add_step("post", test_worker_thread_executor_post);
    tester.add_step("submit", test_worker_thread_executor_submit);
    tester.add_step("bulk_post", test_worker_thread_executor_bulk_post);
    tester.add_step("bulk_submit", test_worker_thread_executor_bulk_submit);

    tester.launch_test();
}
