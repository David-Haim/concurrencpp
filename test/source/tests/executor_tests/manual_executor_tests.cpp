#include "concurrencpp/concurrencpp.h"

#include "tests/all_tests.h"

#include "tests/test_utils/executor_shutdowner.h"

#include "tester/tester.h"
#include "helpers/assertions.h"
#include "helpers/object_observer.h"

#include "concurrencpp/executors/constants.h"

namespace concurrencpp::tests {
    void test_manual_executor_name();

    void test_manual_executor_shutdown_method_access();
    void test_manual_executor_shutdown_coro_raii();
    void test_manual_executor_shutdown_more_than_once();
    void test_manual_executor_shutdown();

    void test_manual_executor_max_concurrency_level();

    void test_manual_executor_post_foreign();
    void test_manual_executor_post_inline();
    void test_manual_executor_post();

    void test_manual_executor_submit_foreign();
    void test_manual_executor_submit_inline();
    void test_manual_executor_submit();

    void test_manual_executor_bulk_post_foreign();
    void test_manual_executor_bulk_post_inline();
    void test_manual_executor_bulk_post();

    void test_manual_executor_bulk_submit_foreign();
    void test_manual_executor_bulk_submit_inline();
    void test_manual_executor_bulk_submit();

    void test_manual_executor_loop_once();
    void test_manual_executor_loop_once_timed();

    void test_manual_executor_loop();

    void test_manual_executor_clear();

    void test_manual_executor_wait_for_task();
    void test_manual_executor_wait_for_task_timed();
}  // namespace concurrencpp::tests

void concurrencpp::tests::test_manual_executor_name() {
    auto executor = std::make_shared<concurrencpp::manual_executor>();
    assert_equal(executor->name, concurrencpp::details::consts::k_manual_executor_name);
}

void concurrencpp::tests::test_manual_executor_shutdown_method_access() {
    auto executor = std::make_shared<manual_executor>();
    assert_false(executor->shutdown_requested());

    executor->shutdown();
    assert_true(executor->shutdown_requested());

    assert_throws<concurrencpp::errors::executor_shutdown>([executor] {
        executor->enqueue(std::experimental::coroutine_handle {});
    });

    assert_throws<concurrencpp::errors::executor_shutdown>([executor] {
        std::experimental::coroutine_handle<> array[4];
        std::span<std::experimental::coroutine_handle<>> span = array;
        executor->enqueue(span);
    });

    assert_throws<concurrencpp::errors::executor_shutdown>([executor] {
        executor->wait_for_task();
    });

    assert_throws<concurrencpp::errors::executor_shutdown>([executor] {
        executor->wait_for_task(std::chrono::milliseconds(100));
    });

    assert_throws<concurrencpp::errors::executor_shutdown>([executor] {
        executor->loop_once();
    });

    assert_throws<concurrencpp::errors::executor_shutdown>([executor] {
        executor->loop(100);
    });
}

void concurrencpp::tests::test_manual_executor_shutdown_coro_raii() {
    object_observer observer;
    const size_t task_count = 1'024;
    auto executor = std::make_shared<manual_executor>();

    std::vector<value_testing_stub> stubs;
    stubs.reserve(task_count);

    for (size_t i = 0; i < task_count; i++) {
        stubs.emplace_back(observer.get_testing_stub(i));
    }

    auto results = executor->bulk_submit<value_testing_stub>(stubs);

    executor->shutdown();
    assert_true(executor->shutdown_requested());

    assert_equal(observer.get_execution_count(), size_t(0));
    assert_equal(observer.get_destruction_count(), task_count);

    for (auto& result : results) {
        assert_throws<concurrencpp::errors::broken_task>([&result] {
            result.get();
        });
    }
}

void concurrencpp::tests::test_manual_executor_shutdown_more_than_once() {
    const size_t task_count = 64;
    auto executor = std::make_shared<manual_executor>();

    for (size_t i = 0; i < task_count; i++) {
        executor->post([] {
        });
    }

    for (size_t i = 0; i < 4; i++) {
        executor->shutdown();
    }
}

void concurrencpp::tests::test_manual_executor_shutdown() {
    test_manual_executor_shutdown_method_access();
    test_manual_executor_shutdown_coro_raii();
    test_manual_executor_shutdown_more_than_once();
}

void concurrencpp::tests::test_manual_executor_max_concurrency_level() {
    auto executor = std::make_shared<manual_executor>();
    executor_shutdowner shutdown(executor);

    assert_equal(executor->max_concurrency_level(), concurrencpp::details::consts::k_manual_executor_max_concurrency_level);
}

void concurrencpp::tests::test_manual_executor_post_foreign() {
    object_observer observer;
    const size_t task_count = 1'024;
    auto executor = std::make_shared<concurrencpp::manual_executor>();

    assert_equal(executor->size(), size_t(0));
    assert_true(executor->empty());

    for (size_t i = 0; i < task_count; i++) {
        executor->post(observer.get_testing_stub());
        assert_equal(executor->size(), 1 + i);
        assert_false(executor->empty());
    }

    // manual executor doesn't execute the tasks automatically, hence manual.
    assert_equal(observer.get_execution_count(), size_t(0));
    assert_equal(observer.get_destruction_count(), size_t(0));

    for (size_t i = 0; i < task_count / 2; i++) {
        assert_true(executor->loop_once());
        assert_equal(observer.get_execution_count(), i + 1);
    }

    executor->shutdown();
    assert_equal(observer.get_destruction_count(), task_count);
}

void concurrencpp::tests::test_manual_executor_post_inline() {
    object_observer observer;
    constexpr size_t task_count = 1'024;
    auto executor = std::make_shared<manual_executor>();

    assert_equal(executor->size(), size_t(0));
    assert_true(executor->empty());

    executor->post([executor, &observer] {
        for (size_t i = 0; i < task_count; i++) {
            executor->post(observer.get_testing_stub());
            assert_equal(executor->size(), 1 + i);
            assert_false(executor->empty());
        }
    });

    assert_true(executor->loop_once());

    assert_equal(observer.get_execution_count(), size_t(0));
    assert_equal(observer.get_destruction_count(), size_t(0));

    for (size_t i = 0; i < task_count / 2; i++) {
        assert_true(executor->loop_once());
        assert_equal(observer.get_execution_count(), i + 1);
    }

    executor->shutdown();

    assert_equal(observer.get_destruction_count(), task_count);
}

void concurrencpp::tests::test_manual_executor_post() {
    test_manual_executor_post_foreign();
    test_manual_executor_post_inline();
}

void concurrencpp::tests::test_manual_executor_submit_foreign() {
    object_observer observer;
    const size_t task_count = 1'024;
    auto executor = std::make_shared<manual_executor>();

    assert_equal(executor->size(), size_t(0));
    assert_true(executor->empty());

    std::vector<result<size_t>> results;
    results.resize(task_count);

    for (size_t i = 0; i < task_count; i++) {
        results[i] = executor->submit(observer.get_testing_stub(i));
    }

    assert_equal(observer.get_execution_count(), size_t(0));
    assert_equal(observer.get_destruction_count(), size_t(0));

    for (size_t i = 0; i < task_count / 2; i++) {
        assert_true(executor->loop_once());
        assert_equal(observer.get_execution_count(), i + 1);
        assert_equal(results[i].get(), i);
    }

    executor->shutdown();
    assert_equal(observer.get_destruction_count(), task_count);
}

void concurrencpp::tests::test_manual_executor_submit_inline() {
    object_observer observer;
    constexpr size_t task_count = 1'024;
    auto executor = std::make_shared<concurrencpp::manual_executor>();

    assert_equal(executor->size(), size_t(0));
    assert_true(executor->empty());

    auto results_res = executor->submit([executor, &observer] {
        std::vector<result<size_t>> results;
        results.resize(task_count);
        for (size_t i = 0; i < task_count; i++) {
            results[i] = executor->submit(observer.get_testing_stub(i));
        }

        return results;
    });

    assert_equal(observer.get_execution_count(), size_t(0));
    assert_equal(observer.get_destruction_count(), size_t(0));

    assert_true(executor->loop_once());
    auto results = results_res.get();

    for (size_t i = 0; i < task_count / 2; i++) {
        assert_true(executor->loop_once());
        assert_equal(observer.get_execution_count(), i + 1);
        assert_equal(results[i].get(), i);
    }

    executor->shutdown();
    assert_equal(observer.get_destruction_count(), task_count);
}

void concurrencpp::tests::test_manual_executor_submit() {
    test_manual_executor_submit_foreign();
    test_manual_executor_submit_inline();
}

void concurrencpp::tests::test_manual_executor_bulk_post_foreign() {
    object_observer observer;
    const size_t task_count = 1'000;
    auto executor = std::make_shared<manual_executor>();

    std::vector<testing_stub> stubs;
    stubs.reserve(task_count);

    for (size_t i = 0; i < task_count; i++) {
        stubs.emplace_back(observer.get_testing_stub());
    }

    executor->bulk_post<testing_stub>(stubs);

    assert_false(executor->empty());
    assert_equal(executor->size(), task_count);

    assert_equal(observer.get_execution_count(), size_t(0));
    assert_equal(observer.get_destruction_count(), size_t(0));

    for (size_t i = 0; i < task_count / 2; i++) {
        assert_true(executor->loop_once());
        assert_equal(observer.get_execution_count(), i + 1);
    }

    executor->shutdown();
    assert_equal(observer.get_destruction_count(), task_count);
}

void concurrencpp::tests::test_manual_executor_bulk_post_inline() {
    object_observer observer;
    constexpr size_t task_count = 1'000;
    auto executor = std::make_shared<manual_executor>();
    executor_shutdowner shutdown(executor);

    executor->post([executor, &observer]() mutable {
        std::vector<testing_stub> stubs;
        stubs.reserve(task_count);

        for (size_t i = 0; i < task_count; i++) {
            stubs.emplace_back(observer.get_testing_stub());
        }
        executor->bulk_post<testing_stub>(stubs);
    });

    assert_true(executor->loop_once());

    assert_equal(observer.get_execution_count(), size_t(0));
    assert_equal(observer.get_destruction_count(), size_t(0));

    for (size_t i = 0; i < task_count / 2; i++) {
        assert_true(executor->loop_once());
        assert_equal(observer.get_execution_count(), i + 1);
    }

    executor->shutdown();
    assert_equal(observer.get_destruction_count(), task_count);
}

void concurrencpp::tests::test_manual_executor_bulk_post() {
    test_manual_executor_bulk_post_foreign();
    test_manual_executor_bulk_post_inline();
}

void concurrencpp::tests::test_manual_executor_bulk_submit_foreign() {
    object_observer observer;
    const size_t task_count = 1'024;
    auto executor = std::make_shared<manual_executor>();
    executor_shutdowner shutdown(executor);

    std::vector<value_testing_stub> stubs;
    stubs.reserve(task_count);

    for (size_t i = 0; i < task_count; i++) {
        stubs.emplace_back(observer.get_testing_stub(i));
    }

    auto results = executor->bulk_submit<value_testing_stub>(stubs);

    assert_false(executor->empty());
    assert_equal(executor->size(), task_count);

    assert_equal(observer.get_execution_count(), size_t(0));
    assert_equal(observer.get_destruction_count(), size_t(0));

    for (size_t i = 0; i < task_count / 2; i++) {
        assert_true(executor->loop_once());
        assert_equal(observer.get_execution_count(), i + 1);
    }

    executor->shutdown();
    assert_equal(observer.get_destruction_count(), task_count);
}

void concurrencpp::tests::test_manual_executor_bulk_submit_inline() {
    object_observer observer;
    constexpr size_t task_count = 1'024;
    auto executor = std::make_shared<manual_executor>();
    executor_shutdowner shutdown(executor);

    auto results_res = executor->submit([executor, &observer] {
        std::vector<value_testing_stub> stubs;
        stubs.reserve(task_count);

        for (size_t i = 0; i < task_count; i++) {
            stubs.emplace_back(observer.get_testing_stub(i));
        }

        return executor->bulk_submit<value_testing_stub>(stubs);
    });

    assert_true(executor->loop_once());

    assert_equal(observer.get_execution_count(), size_t(0));
    assert_equal(observer.get_destruction_count(), size_t(0));

    auto results = results_res.get();
    for (size_t i = 0; i < task_count / 2; i++) {
        assert_true(executor->loop_once());
        assert_equal(observer.get_execution_count(), i + 1);
        assert_equal(results[i].get(), i);
    }

    executor->shutdown();
    assert_equal(observer.get_destruction_count(), task_count);
}

void concurrencpp::tests::test_manual_executor_bulk_submit() {
    test_manual_executor_bulk_submit_foreign();
    test_manual_executor_bulk_submit_inline();
}

void concurrencpp::tests::test_manual_executor_loop_once() {
    object_observer observer;
    const size_t task_count = 1'024;
    auto executor = std::make_shared<concurrencpp::manual_executor>();
    executor_shutdowner shutdown(executor);

    assert_equal(executor->size(), size_t(0));
    assert_true(executor->empty());

    for (size_t i = 0; i < 10; i++) {
        assert_false(executor->loop_once());
    }

    std::vector<result<size_t>> results;
    results.resize(task_count);

    for (size_t i = 0; i < task_count; i++) {
        results[i] = executor->submit(observer.get_testing_stub(i));
    }

    assert_equal(observer.get_execution_count(), size_t(0));
    assert_equal(observer.get_destruction_count(), size_t(0));

    for (size_t i = 0; i < task_count; i++) {
        assert_true(executor->loop_once());
        assert_equal(observer.get_execution_count(), i + 1);
        assert_equal(executor->size(), task_count - (i + 1));
        assert_equal(results[i].get(), i);
    }

    assert_equal(observer.get_destruction_count(), task_count);

    const auto& execution_map = observer.get_execution_map();

    assert_equal(execution_map.size(), size_t(1));
    assert_equal(execution_map.begin()->first, concurrencpp::details::thread::get_current_virtual_id());

    for (size_t i = 0; i < 10; i++) {
        assert_false(executor->loop_once());
    }
}

void concurrencpp::tests::test_manual_executor_loop_once_timed() {
    auto executor = std::make_shared<concurrencpp::manual_executor>();
    executor_shutdowner shutdown(executor);
    object_observer observer;
    const auto waiting_time = 20;

    // case 1: timeout
    {
        for (size_t i = 0; i < 10; i++) {
            const auto before = std::chrono::high_resolution_clock::now();
            assert_false(executor->loop_once(std::chrono::milliseconds(waiting_time)));
            const auto after = std::chrono::high_resolution_clock::now();
            const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(after - before).count();
            assert_bigger_equal(ms, waiting_time);
        }
    }

    // case 2: tasks already exist
    {
        executor->post(observer.get_testing_stub());
        const auto before = std::chrono::high_resolution_clock::now();
        assert_true(executor->loop_once(std::chrono::milliseconds(waiting_time)));
        const auto after = std::chrono::high_resolution_clock::now();
        const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(after - before).count();
        assert_smaller_equal(ms, 5);
        assert_equal(observer.get_execution_count(), size_t(1));
        assert_equal(observer.get_destruction_count(), size_t(1));
    }

    // case 3: goes to sleep, then woken by an incoming task
    {
        const auto later = std::chrono::high_resolution_clock::now() + std::chrono::seconds(2);

        std::thread thread([executor, later, stub = observer.get_testing_stub()]() mutable {
            std::this_thread::sleep_until(later);
            executor->post(std::move(stub));
        });

        assert_true(executor->loop_once(std::chrono::seconds(100)));
        const auto now = std::chrono::high_resolution_clock::now();

        assert_bigger_equal(now, later);
        assert_smaller_equal(now, later + std::chrono::seconds(2));

        thread.join();
    }
}

void concurrencpp::tests::test_manual_executor_loop() {
    object_observer observer;
    const size_t task_count = 1'000;
    auto executor = std::make_shared<concurrencpp::manual_executor>();
    executor_shutdowner shutdown(executor);

    assert_equal(executor->size(), size_t(0));
    assert_true(executor->empty());

    assert_equal(executor->loop(100), size_t(0));

    std::vector<result<size_t>> results;
    results.resize(task_count);

    for (size_t i = 0; i < task_count; i++) {
        results[i] = executor->submit(observer.get_testing_stub(i));
    }

    assert_equal(observer.get_execution_count(), size_t(0));
    assert_equal(observer.get_destruction_count(), size_t(0));

    const size_t chunk_size = 150;
    const auto cycles = task_count / chunk_size;
    const auto remained = task_count - (cycles * chunk_size);

    for (size_t i = 0; i < cycles; i++) {
        const auto executed = executor->loop(chunk_size);
        assert_equal(executed, chunk_size);

        const auto total_executed = (i + 1) * chunk_size;
        assert_equal(observer.get_execution_count(), total_executed);
        assert_equal(observer.get_destruction_count(), total_executed);
        assert_equal(executor->size(), task_count - total_executed);
    }

    // execute the remaining 100 tasks
    const auto executed = executor->loop(chunk_size);
    assert_equal(executed, remained);

    assert_equal(observer.get_execution_count(), task_count);
    assert_equal(observer.get_destruction_count(), task_count);

    assert_true(executor->empty());
    assert_equal(executor->size(), size_t(0));

    const auto& execution_map = observer.get_execution_map();

    assert_equal(execution_map.size(), size_t(1));
    assert_equal(execution_map.begin()->first, concurrencpp::details::thread::get_current_virtual_id());

    for (size_t i = 0; i < task_count; i++) {
        assert_equal(results[i].get(), i);
    }
}

void concurrencpp::tests::test_manual_executor_clear() {
    object_observer observer;
    const size_t task_count = 100;
    auto executor = std::make_shared<concurrencpp::manual_executor>();
    executor_shutdowner shutdown(executor);

    assert_equal(executor->clear(), size_t(0));

    std::vector<result<size_t>> results;
    results.resize(task_count);

    for (size_t i = 0; i < task_count; i++) {
        results[i] = executor->submit(observer.get_testing_stub(i));
    }

    assert_equal(executor->clear(), task_count);
    assert_true(executor->empty());
    assert_equal(executor->size(), size_t(0));
    assert_equal(observer.get_execution_count(), size_t(0));
    assert_equal(observer.get_destruction_count(), task_count);

    for (auto& result : results) {
        assert_throws<concurrencpp::errors::broken_task>([&result]() mutable {
            result.get();
        });
    }
}

void concurrencpp::tests::test_manual_executor_wait_for_task() {
    auto executor = std::make_shared<concurrencpp::manual_executor>();
    executor_shutdowner shutdown(executor);
    auto enqueuing_time = std::chrono::system_clock::now() + std::chrono::milliseconds(2'500);

    std::thread enqueuing_thread([executor, enqueuing_time]() mutable {
        std::this_thread::sleep_until(enqueuing_time);
        executor->post([] {
        });
    });

    executor->wait_for_task();
    assert_bigger_equal(std::chrono::system_clock::now(), enqueuing_time);

    enqueuing_thread.join();
}

void concurrencpp::tests::test_manual_executor_wait_for_task_timed() {
    auto executor = std::make_shared<concurrencpp::manual_executor>();
    executor_shutdowner shutdown(executor);
    const auto waiting_time = std::chrono::milliseconds(200);

    for (size_t i = 0; i < 10; i++) {
        const auto before = std::chrono::system_clock::now();
        const auto task_found = executor->wait_for_task(waiting_time);
        const auto after = std::chrono::system_clock::now();
        const auto time_elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(after - before);

        assert_false(task_found);
        assert_bigger_equal(time_elapsed, waiting_time);
    }

    auto enqueuing_time = std::chrono::system_clock::now() + std::chrono::milliseconds(2'500);

    std::thread enqueuing_thread([executor, enqueuing_time]() mutable {
        std::this_thread::sleep_until(enqueuing_time);
        executor->post([] {
        });
    });

    const auto task_found = executor->wait_for_task(std::chrono::seconds(10));
    assert_true(task_found);
    assert_bigger_equal(std::chrono::system_clock::now(), enqueuing_time);

    enqueuing_thread.join();
}

void concurrencpp::tests::test_manual_executor() {
    tester tester("manual_executor test");

    tester.add_step("name", test_manual_executor_name);
    tester.add_step("shutdown", test_manual_executor_shutdown);
    tester.add_step("max_concurrency_level", test_manual_executor_max_concurrency_level);
    tester.add_step("post", test_manual_executor_post);
    tester.add_step("submit", test_manual_executor_submit);
    tester.add_step("bulk_post", test_manual_executor_bulk_post);
    tester.add_step("bulk_submit", test_manual_executor_bulk_submit);
    tester.add_step("loop_once", test_manual_executor_loop_once);
    tester.add_step("loop_once (ms)", test_manual_executor_loop_once_timed);
    tester.add_step("loop", test_manual_executor_loop);
    tester.add_step("wait_for_task", test_manual_executor_wait_for_task);
    tester.add_step("wait_for_task (ms)", test_manual_executor_wait_for_task_timed);
    tester.add_step("clear", test_manual_executor_clear);

    tester.launch_test();
}
