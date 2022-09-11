#include "concurrencpp/concurrencpp.h"

#include "infra/tester.h"
#include "infra/assertions.h"
#include "utils/object_observer.h"
#include "utils/test_generators.h"
#include "utils/test_ready_result.h"
#include "utils/executor_shutdowner.h"

namespace concurrencpp::tests {
    void test_manual_executor_name();

    void test_manual_executor_shutdown_method_access();
    void test_manual_executor_shutdown_more_than_once();
    void test_manual_executor_shutdown();

    void test_manual_executor_max_concurrency_level();

    void test_manual_executor_post_exception();
    void test_manual_executor_post_foreign();
    void test_manual_executor_post_inline();
    void test_manual_executor_post();

    void test_manual_executor_submit_exception();
    void test_manual_executor_submit_foreign();
    void test_manual_executor_submit_inline();
    void test_manual_executor_submit();

    void test_manual_executor_bulk_post_exception();
    void test_manual_executor_bulk_post_foreign();
    void test_manual_executor_bulk_post_inline();
    void test_manual_executor_bulk_post();

    void test_manual_executor_bulk_submit_exception();
    void test_manual_executor_bulk_submit_foreign();
    void test_manual_executor_bulk_submit_inline();
    void test_manual_executor_bulk_submit();

    void test_manual_executor_loop_once();
    void test_manual_executor_loop_once_for();
    void test_manual_executor_loop_once_until();

    void test_manual_executor_loop();
    void test_manual_executor_loop_for();
    void test_manual_executor_loop_until();

    void test_manual_executor_clear();

    void test_manual_executor_wait_for_task();
    void test_manual_executor_wait_for_task_for();
    void test_manual_executor_wait_for_task_until();

    void test_manual_executor_wait_for_tasks();
    void test_manual_executor_wait_for_tasks_for();
    void test_manual_executor_wait_for_tasks_until();

    void assert_executed_locally(const std::unordered_map<size_t, size_t>& execution_map) {
        assert_equal(execution_map.size(), static_cast<size_t>(1));  // only one thread executed the tasks
        assert_equal(execution_map.begin()->first, concurrencpp::details::thread::get_current_virtual_id());  // and it's this thread.
    }
}  // namespace concurrencpp::tests

using namespace std::chrono;

void concurrencpp::tests::test_manual_executor_name() {
    auto executor = std::make_shared<concurrencpp::manual_executor>();
    assert_equal(executor->name, concurrencpp::details::consts::k_manual_executor_name);
}

void concurrencpp::tests::test_manual_executor_shutdown_method_access() {
    auto executor = std::make_shared<manual_executor>();
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

    assert_throws<concurrencpp::errors::runtime_shutdown>([executor] {
        executor->clear();
    });

    assert_throws<concurrencpp::errors::runtime_shutdown>([executor] {
        executor->loop_once();
    });

    assert_throws<concurrencpp::errors::runtime_shutdown>([executor] {
        executor->loop_once_for(milliseconds(100));
    });

    assert_throws<concurrencpp::errors::runtime_shutdown>([executor] {
        executor->loop_once_until(high_resolution_clock::now() + milliseconds(100));
    });

    assert_throws<concurrencpp::errors::runtime_shutdown>([executor] {
        executor->loop(100);
    });

    assert_throws<concurrencpp::errors::runtime_shutdown>([executor] {
        executor->loop_for(100, milliseconds(100));
    });

    assert_throws<concurrencpp::errors::runtime_shutdown>([executor] {
        executor->loop_until(100, high_resolution_clock::now() + milliseconds(100));
    });

    assert_throws<concurrencpp::errors::runtime_shutdown>([executor] {
        executor->wait_for_task();
    });

    assert_throws<concurrencpp::errors::runtime_shutdown>([executor] {
        executor->wait_for_task_for(milliseconds(100));
    });

    assert_throws<concurrencpp::errors::runtime_shutdown>([executor] {
        executor->wait_for_task_until(high_resolution_clock::now() + milliseconds(100));
    });

    assert_throws<concurrencpp::errors::runtime_shutdown>([executor] {
        executor->wait_for_tasks(8);
    });

    assert_throws<concurrencpp::errors::runtime_shutdown>([executor] {
        executor->wait_for_tasks_for(8, milliseconds(100));
    });

    assert_throws<concurrencpp::errors::runtime_shutdown>([executor] {
        executor->wait_for_tasks_until(8, high_resolution_clock::now() + milliseconds(100));
    });
}

void concurrencpp::tests::test_manual_executor_shutdown_more_than_once() {
    auto executor = std::make_shared<manual_executor>();
    for (size_t i = 0; i < 4; i++) {
        executor->shutdown();
    }
}

void concurrencpp::tests::test_manual_executor_shutdown() {
    test_manual_executor_shutdown_method_access();
    test_manual_executor_shutdown_more_than_once();
}

void concurrencpp::tests::test_manual_executor_max_concurrency_level() {
    auto executor = std::make_shared<manual_executor>();
    executor_shutdowner shutdown(executor);

    assert_equal(executor->max_concurrency_level(), concurrencpp::details::consts::k_manual_executor_max_concurrency_level);
}

void concurrencpp::tests::test_manual_executor_post_exception() {
    auto executor = std::make_shared<concurrencpp::manual_executor>();
    executor_shutdowner shutdown(executor);

    executor->post([] {
        throw std::runtime_error("");
    });

    assert_true(executor->loop_once());
}

void concurrencpp::tests::test_manual_executor_post_foreign() {
    object_observer observer;
    const size_t task_count = 1'024;
    auto executor = std::make_shared<concurrencpp::manual_executor>();

    assert_equal(executor->size(), static_cast<size_t>(0));
    assert_true(executor->empty());

    for (size_t i = 0; i < task_count; i++) {
        executor->post(observer.get_testing_stub());
        assert_equal(executor->size(), 1 + i);
        assert_false(executor->empty());
    }

    // manual executor doesn't execute the tasks automatically, hence manual.
    assert_equal(observer.get_execution_count(), static_cast<size_t>(0));
    assert_equal(observer.get_destruction_count(), static_cast<size_t>(0));

    for (size_t i = 0; i < task_count / 2; i++) {
        assert_true(executor->loop_once());
        assert_equal(observer.get_execution_count(), i + 1);
    }

    executor->shutdown();
    assert_equal(observer.get_destruction_count(), task_count);

    assert_executed_locally(observer.get_execution_map());
}

void concurrencpp::tests::test_manual_executor_post_inline() {
    object_observer observer;
    constexpr size_t task_count = 1'024;
    auto executor = std::make_shared<manual_executor>();

    assert_equal(executor->size(), static_cast<size_t>(0));
    assert_true(executor->empty());

    executor->post([executor, &observer] {
        for (size_t i = 0; i < task_count; i++) {
            executor->post(observer.get_testing_stub());
            assert_equal(executor->size(), 1 + i);
            assert_false(executor->empty());
        }
    });

    // the tasks are not enqueued yet, only the spawning task is.
    assert_equal(executor->size(), static_cast<size_t>(1));
    assert_false(executor->empty());

    assert_true(executor->loop_once());
    assert_equal(executor->size(), task_count);
    assert_false(executor->empty());

    assert_equal(observer.get_execution_count(), static_cast<size_t>(0));
    assert_equal(observer.get_destruction_count(), static_cast<size_t>(0));

    for (size_t i = 0; i < task_count / 2; i++) {
        assert_true(executor->loop_once());
        assert_equal(observer.get_execution_count(), i + 1);
        assert_equal(observer.get_destruction_count(), i + 1);
    }

    executor->shutdown();
    assert_equal(observer.get_destruction_count(), task_count);

    assert_executed_locally(observer.get_execution_map());
}

void concurrencpp::tests::test_manual_executor_post() {
    test_manual_executor_post_exception();
    test_manual_executor_post_foreign();
    test_manual_executor_post_inline();
}

void concurrencpp::tests::test_manual_executor_submit_exception() {
    auto executor = std::make_shared<manual_executor>();
    executor_shutdowner shutdowner(executor);

    constexpr intptr_t id = 12345;

    auto result = executor->submit([id] {
        throw custom_exception(id);
    });

    assert_true(executor->loop_once());

    result.wait();
    test_ready_result_custom_exception(std::move(result), id);
}

void concurrencpp::tests::test_manual_executor_submit_foreign() {
    object_observer observer;
    const size_t task_count = 1'024;
    auto executor = std::make_shared<manual_executor>();

    assert_equal(executor->size(), static_cast<size_t>(0));
    assert_true(executor->empty());

    std::vector<result<size_t>> results;
    results.resize(task_count);

    for (size_t i = 0; i < task_count; i++) {
        results[i] = executor->submit(observer.get_testing_stub(i));
    }

    assert_equal(observer.get_execution_count(), static_cast<size_t>(0));
    assert_equal(observer.get_destruction_count(), static_cast<size_t>(0));

    for (size_t i = 0; i < task_count / 2; i++) {
        assert_true(executor->loop_once());
        assert_equal(results[i].get(), i);
        assert_equal(observer.get_execution_count(), i + 1);
        assert_equal(observer.get_destruction_count(), i + 1);
    }

    executor->shutdown();

    assert_executed_locally(observer.get_execution_map());

    for (size_t i = task_count / 2; i < task_count; i++) {
        assert_throws<errors::broken_task>([&, i] {
            results[i].get();
        });
    }

    assert_equal(observer.get_destruction_count(), task_count);
}

void concurrencpp::tests::test_manual_executor_submit_inline() {
    object_observer observer;
    constexpr size_t task_count = 1'024;
    auto executor = std::make_shared<concurrencpp::manual_executor>();

    assert_equal(executor->size(), static_cast<size_t>(0));
    assert_true(executor->empty());

    auto results_res = executor->submit([executor, &observer] {
        std::vector<result<size_t>> results;
        results.resize(task_count);
        for (size_t i = 0; i < task_count; i++) {
            results[i] = executor->submit(observer.get_testing_stub(i));
        }

        return results;
    });

    // the tasks are not enqueued yet, only the spawning task is.
    assert_equal(executor->size(), static_cast<size_t>(1));
    assert_false(executor->empty());

    assert_true(executor->loop_once());
    assert_equal(executor->size(), task_count);
    assert_false(executor->empty());

    assert_equal(observer.get_execution_count(), static_cast<size_t>(0));
    assert_equal(observer.get_destruction_count(), static_cast<size_t>(0));

    assert_equal(results_res.status(), result_status::value);
    auto results = results_res.get();

    for (size_t i = 0; i < task_count / 2; i++) {
        assert_true(executor->loop_once());
        assert_equal(results[i].get(), i);
        assert_equal(observer.get_execution_count(), i + 1);
        assert_equal(observer.get_destruction_count(), i + 1);
    }

    executor->shutdown();

    assert_executed_locally(observer.get_execution_map());

    for (size_t i = task_count / 2; i < task_count; i++) {
        assert_throws<errors::broken_task>([&, i] {
            results[i].get();
        });
    }

    assert_equal(observer.get_destruction_count(), task_count);
}

void concurrencpp::tests::test_manual_executor_submit() {
    test_manual_executor_submit_exception();
    test_manual_executor_submit_foreign();
    test_manual_executor_submit_inline();
}

void concurrencpp::tests::test_manual_executor_bulk_post_exception() {
    auto executor = std::make_shared<manual_executor>();
    executor_shutdowner shutdown(executor);

    auto thrower = [] {
        throw std::runtime_error("");
    };

    std::vector<decltype(thrower)> tasks;
    tasks.resize(4);

    executor->bulk_post<decltype(thrower)>(tasks);
    assert_equal(executor->loop(4), 4);
}

void concurrencpp::tests::test_manual_executor_bulk_post_foreign() {
    object_observer observer;
    const size_t task_count = 1'024;
    auto executor = std::make_shared<manual_executor>();

    std::vector<testing_stub> stubs;
    stubs.reserve(task_count);

    for (size_t i = 0; i < task_count; i++) {
        stubs.emplace_back(observer.get_testing_stub());
    }

    executor->bulk_post<testing_stub>(stubs);

    assert_equal(executor->size(), task_count);
    assert_false(executor->empty());

    assert_equal(observer.get_execution_count(), static_cast<size_t>(0));
    assert_equal(observer.get_destruction_count(), static_cast<size_t>(0));

    for (size_t i = 0; i < task_count / 2; i++) {
        assert_true(executor->loop_once());
        assert_equal(observer.get_execution_count(), i + 1);
        assert_equal(observer.get_destruction_count(), i + 1);
    }

    executor->shutdown();
    assert_equal(observer.get_destruction_count(), task_count);

    assert_executed_locally(observer.get_execution_map());
}

void concurrencpp::tests::test_manual_executor_bulk_post_inline() {
    object_observer observer;
    constexpr size_t task_count = 1'024;
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

    // the tasks are not enqueued yet, only the spawning task is.
    assert_equal(executor->size(), static_cast<size_t>(1));
    assert_false(executor->empty());

    assert_true(executor->loop_once());
    assert_equal(executor->size(), task_count);
    assert_false(executor->empty());

    assert_equal(observer.get_execution_count(), static_cast<size_t>(0));
    assert_equal(observer.get_destruction_count(), static_cast<size_t>(0));

    for (size_t i = 0; i < task_count / 2; i++) {
        assert_true(executor->loop_once());
        assert_equal(observer.get_execution_count(), i + 1);
        assert_equal(observer.get_destruction_count(), i + 1);
    }

    executor->shutdown();
    assert_equal(observer.get_destruction_count(), task_count);

    assert_executed_locally(observer.get_execution_map());
}

void concurrencpp::tests::test_manual_executor_bulk_post() {
    test_manual_executor_bulk_post_exception();
    test_manual_executor_bulk_post_foreign();
    test_manual_executor_bulk_post_inline();
}

void concurrencpp::tests::test_manual_executor_bulk_submit_exception() {
    auto executor = std::make_shared<manual_executor>();
    executor_shutdowner shutdown(executor);
    constexpr intptr_t id = 12345;

    auto thrower = [] {
        throw custom_exception(id);
    };

    std::vector<decltype(thrower)> tasks;
    tasks.resize(4, thrower);

    auto results = executor->bulk_submit<decltype(thrower)>(tasks);
    assert_equal(executor->loop(4), 4);

    for (auto& result : results) {
        result.wait();
        test_ready_result_custom_exception(std::move(result), id);
    }
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

    assert_equal(observer.get_execution_count(), static_cast<size_t>(0));
    assert_equal(observer.get_destruction_count(), static_cast<size_t>(0));

    for (size_t i = 0; i < task_count / 2; i++) {
        assert_true(executor->loop_once());
        assert_equal(results[i].get(), i);
        assert_equal(observer.get_execution_count(), i + 1);
        assert_equal(observer.get_destruction_count(), i + 1);
    }

    executor->shutdown();

    assert_executed_locally(observer.get_execution_map());

    for (size_t i = task_count / 2; i < task_count; i++) {
        assert_throws<errors::broken_task>([&, i] {
            results[i].get();
        });
    }

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

    // the tasks are not enqueued yet, only the spawning task is.
    assert_equal(executor->size(), static_cast<size_t>(1));
    assert_false(executor->empty());

    assert_true(executor->loop_once());
    assert_equal(executor->size(), task_count);
    assert_false(executor->empty());

    assert_equal(observer.get_execution_count(), static_cast<size_t>(0));
    assert_equal(observer.get_destruction_count(), static_cast<size_t>(0));

    assert_equal(results_res.status(), result_status::value);
    auto results = results_res.get();

    for (size_t i = 0; i < task_count / 2; i++) {
        assert_true(executor->loop_once());
        assert_equal(results[i].get(), i);
        assert_equal(observer.get_execution_count(), i + 1);
        assert_equal(observer.get_destruction_count(), i + 1);
    }

    executor->shutdown();

    assert_executed_locally(observer.get_execution_map());

    for (size_t i = task_count / 2; i < task_count; i++) {
        assert_throws<errors::broken_task>([&, i] {
            results[i].get();
        });
    }

    assert_equal(observer.get_destruction_count(), task_count);
}

void concurrencpp::tests::test_manual_executor_bulk_submit() {
    test_manual_executor_bulk_submit_exception();
    test_manual_executor_bulk_submit_foreign();
    test_manual_executor_bulk_submit_inline();
}

void concurrencpp::tests::test_manual_executor_loop_once() {
    object_observer observer;
    const size_t task_count = 1'024;
    auto executor = std::make_shared<concurrencpp::manual_executor>();
    executor_shutdowner shutdown(executor);

    assert_equal(executor->size(), static_cast<size_t>(0));
    assert_true(executor->empty());

    for (size_t i = 0; i < 10; i++) {
        assert_false(executor->loop_once());
    }

    std::vector<result<size_t>> results;
    results.resize(task_count);

    for (size_t i = 0; i < task_count; i++) {
        results[i] = executor->submit(observer.get_testing_stub(i));
    }

    assert_equal(observer.get_execution_count(), static_cast<size_t>(0));
    assert_equal(observer.get_destruction_count(), static_cast<size_t>(0));

    for (size_t i = 0; i < task_count; i++) {
        assert_true(executor->loop_once());
        assert_equal(results[i].get(), i);
        assert_equal(observer.get_execution_count(), i + 1);
        assert_equal(observer.get_destruction_count(), i + 1);
        assert_equal(executor->size(), task_count - (i + 1));
    }

    assert_executed_locally(observer.get_execution_map());

    for (size_t i = 0; i < 10; i++) {
        assert_false(executor->loop_once());
    }
}

void concurrencpp::tests::test_manual_executor_loop_once_for() {
    // case 1: timeout
    {
        auto executor = std::make_shared<concurrencpp::manual_executor>();
        const auto waiting_time_ms = milliseconds(50);
        executor_shutdowner shutdown(executor);

        for (size_t i = 0; i < 10; i++) {
            const auto before = high_resolution_clock::now();
            assert_false(executor->loop_once_for(waiting_time_ms));
            const auto after = high_resolution_clock::now();
            const auto ms_elapsed = std::chrono::duration_cast<milliseconds>(after - before);
            assert_bigger_equal(ms_elapsed, waiting_time_ms);
        }
    }

    // case 2: tasks already exist
    {
        auto executor = std::make_shared<concurrencpp::manual_executor>();
        const auto waiting_time_ms = milliseconds(150);
        executor_shutdowner shutdown(executor);
        object_observer observer;

        executor->post(observer.get_testing_stub());
        const auto before = high_resolution_clock::now();
        assert_true(executor->loop_once_for(waiting_time_ms));
        const auto after = high_resolution_clock::now();
        const auto ms_elapsed = std::chrono::duration_cast<milliseconds>(after - before).count();
        assert_smaller_equal(ms_elapsed, 5);
        assert_equal(observer.get_execution_count(), static_cast<size_t>(1));
        assert_equal(observer.get_destruction_count(), static_cast<size_t>(1));
        assert_executed_locally(observer.get_execution_map());
    }

    // case 3: goes to sleep, then woken by an incoming task
    {
        auto executor = std::make_shared<concurrencpp::manual_executor>();
        executor_shutdowner shutdown(executor);
        object_observer observer;
        const auto enqueue_time = high_resolution_clock::now() + milliseconds(150);

        std::thread thread([executor, enqueue_time, &observer]() mutable {
            std::this_thread::sleep_until(enqueue_time);
            executor->post(observer.get_testing_stub());
        });

        assert_true(executor->loop_once_for(seconds(10)));
        const auto now = high_resolution_clock::now();

        assert_bigger_equal(now, enqueue_time);
        assert_smaller_equal(now, enqueue_time + seconds(1));

        thread.join();

        assert_executed_locally(observer.get_execution_map());
    }

    // case 4: goes to sleep, then woken by a shutdown interrupt
    {
        auto executor = std::make_shared<concurrencpp::manual_executor>();
        executor_shutdowner shutdown(executor);
        const auto shutdown_time = high_resolution_clock::now() + milliseconds(150);

        std::thread thread([executor, shutdown_time]() mutable {
            std::this_thread::sleep_until(shutdown_time);
            executor->shutdown();
        });

        assert_throws<errors::runtime_shutdown>([executor] {
            executor->loop_once_for(seconds(10));
        });

        const auto now = high_resolution_clock::now();
        assert_bigger_equal(now, shutdown_time);
        assert_smaller_equal(now, shutdown_time + seconds(1));

        thread.join();
    }
}

void concurrencpp::tests::test_manual_executor_loop_once_until() {
    // case 1: timeout
    {
        auto executor = std::make_shared<concurrencpp::manual_executor>();
        executor_shutdowner shutdown(executor);

        for (size_t i = 0; i < 10; i++) {
            const auto max_waiting_time_point = high_resolution_clock::now() + milliseconds(50);
            const auto before = high_resolution_clock::now();
            assert_false(executor->loop_once_until(max_waiting_time_point));
            const auto now = high_resolution_clock::now();
            assert_bigger_equal(now, max_waiting_time_point);
        }
    }

    // case 2: tasks already exist
    {
        auto executor = std::make_shared<concurrencpp::manual_executor>();
        executor_shutdowner shutdown(executor);
        object_observer observer;

        const auto max_waiting_time_point = high_resolution_clock::now() + milliseconds(150);
        executor->post(observer.get_testing_stub());
        const auto before = high_resolution_clock::now();
        assert_true(executor->loop_once_until(max_waiting_time_point));
        const auto after = high_resolution_clock::now();
        const auto ms_elapsed = std::chrono::duration_cast<milliseconds>(after - before).count();
        assert_smaller_equal(ms_elapsed, 5);
        assert_equal(observer.get_execution_count(), static_cast<size_t>(1));
        assert_equal(observer.get_destruction_count(), static_cast<size_t>(1));
        assert_executed_locally(observer.get_execution_map());
    }

    // case 3: goes to sleep, then woken by an incoming task
    {
        auto executor = std::make_shared<concurrencpp::manual_executor>();
        executor_shutdowner shutdown(executor);
        object_observer observer;
        const auto enqueue_time = high_resolution_clock::now() + milliseconds(150);

        std::thread thread([executor, enqueue_time, &observer]() mutable {
            std::this_thread::sleep_until(enqueue_time);
            executor->post(observer.get_testing_stub());
        });

        const auto max_looping_time_point = high_resolution_clock::now() + seconds(10);
        assert_true(executor->loop_once_until(max_looping_time_point));
        const auto now = high_resolution_clock::now();

        assert_bigger_equal(now, enqueue_time);
        assert_smaller_equal(now, enqueue_time + seconds(1));

        thread.join();

        assert_executed_locally(observer.get_execution_map());
    }

    // case 4: goes to sleep, then woken by a shutdown interrupt
    {
        auto executor = std::make_shared<concurrencpp::manual_executor>();
        executor_shutdowner shutdown(executor);
        const auto shutdown_time = high_resolution_clock::now() + milliseconds(150);

        std::thread thread([executor, shutdown_time]() mutable {
            std::this_thread::sleep_until(shutdown_time);
            executor->shutdown();
        });

        assert_throws<errors::runtime_shutdown>([executor] {
            const auto max_looping_time_point = high_resolution_clock::now() + seconds(10);
            executor->loop_once_until(max_looping_time_point);
        });

        const auto now = high_resolution_clock::now();

        assert_bigger_equal(now, shutdown_time);
        assert_smaller_equal(now, shutdown_time + seconds(1));

        thread.join();
    }
}

void concurrencpp::tests::test_manual_executor_loop() {
    object_observer observer;
    const size_t task_count = 1'024;
    auto executor = std::make_shared<concurrencpp::manual_executor>();
    executor_shutdowner shutdown(executor);

    assert_equal(executor->size(), static_cast<size_t>(0));
    assert_true(executor->empty());

    assert_equal(executor->loop(100), static_cast<size_t>(0));

    std::vector<result<size_t>> results;
    results.resize(task_count);

    for (size_t i = 0; i < task_count; i++) {
        results[i] = executor->submit(observer.get_testing_stub(i));
    }

    assert_equal(observer.get_execution_count(), static_cast<size_t>(0));
    assert_equal(observer.get_destruction_count(), static_cast<size_t>(0));

    const size_t chunk_size = 150;
    const auto cycles = task_count / chunk_size;
    const auto remained = task_count % (cycles * chunk_size);

    for (size_t i = 0; i < cycles; i++) {
        const auto executed = executor->loop(chunk_size);
        assert_equal(executed, chunk_size);

        const auto total_executed = (i + 1) * chunk_size;
        assert_equal(observer.get_execution_count(), total_executed);
        assert_equal(executor->size(), task_count - total_executed);
    }

    // execute the remaining tasks
    const auto executed = executor->loop(chunk_size);
    assert_equal(executed, remained);

    assert_equal(observer.get_execution_count(), task_count);

    assert_true(executor->empty());
    assert_equal(executor->size(), static_cast<size_t>(0));

    assert_equal(executor->loop(100), static_cast<size_t>(0));

    assert_executed_locally(observer.get_execution_map());

    for (size_t i = 0; i < task_count; i++) {
        assert_equal(results[i].get(), i);
    }

    assert_equal(observer.get_destruction_count(), task_count);
}

void concurrencpp::tests::test_manual_executor_loop_for() {
    // when max_count == 0, the function returns immediately
    {
        object_observer observer;
        auto executor = std::make_shared<concurrencpp::manual_executor>();
        executor_shutdowner shutdown(executor);

        const auto before = high_resolution_clock::now();
        const auto executed = executor->loop_for(0, seconds(10));
        const auto after = high_resolution_clock::now();
        const auto ms_elapsed = duration_cast<milliseconds>(after - before);
        assert_equal(executed, static_cast<size_t>(0));
        assert_smaller_equal(ms_elapsed, milliseconds(5));
    }

    // when max_waiting_time == 0ms, the function behaves like manual_executor::loop
    {
        object_observer observer;
        auto executor = std::make_shared<concurrencpp::manual_executor>();
        executor_shutdowner shutdown(executor);
        assert_equal(executor->loop_for(100, milliseconds(0)), static_cast<size_t>(0));

        const size_t task_count = 100;
        for (size_t i = 0; i < task_count; i++) {
            executor->post(observer.get_testing_stub());
        }

        for (size_t i = 0; i < (task_count - 2) / 2; i++) {
            const auto executed = executor->loop_for(2, milliseconds(0));
            assert_equal(executed, 2);
            assert_equal(observer.get_execution_count(), (i + 1) * 2);
            assert_equal(observer.get_destruction_count(), (i + 1) * 2);
        }

        assert_equal(executor->loop_for(10, milliseconds(0)), 2);
        assert_equal(observer.get_execution_count(), 100);
        assert_equal(observer.get_destruction_count(), 100);

        assert_equal(executor->loop_for(10, milliseconds(0)), 0);

        assert_executed_locally(observer.get_execution_map());
    }

    // if max_count is reached, the function returns
    {
        object_observer observer;
        auto executor = std::make_shared<concurrencpp::manual_executor>();
        executor_shutdowner shutdown(executor);
        const auto max_looping_time = seconds(10);
        const auto enqueueing_interval = milliseconds(100);
        const size_t max_count = 8;

        std::thread enqueuer([max_count, enqueueing_interval, executor, &observer] {
            for (size_t i = 0; i < max_count + 1; i++) {
                std::this_thread::sleep_for(enqueueing_interval);
                executor->post(observer.get_testing_stub());
            }
        });

        const auto before = high_resolution_clock::now();
        const auto executed = executor->loop_for(max_count, max_looping_time);
        const auto after = high_resolution_clock::now();
        const auto ms_elapsed = duration_cast<milliseconds>(after - before);

        assert_equal(executed, max_count);
        assert_bigger_equal(ms_elapsed, max_count * enqueueing_interval);
        assert_smaller(ms_elapsed, max_count * enqueueing_interval + seconds(1));

        enqueuer.join();
    }

    // if shutdown requested, the function returns and throws
    {
        auto executor = std::make_shared<concurrencpp::manual_executor>();
        executor_shutdowner shutdown(executor);
        const auto shutdown_time = high_resolution_clock::now() + milliseconds(150);

        std::thread thread([executor, shutdown_time]() mutable {
            std::this_thread::sleep_until(shutdown_time);
            executor->shutdown();
        });

        assert_throws<errors::runtime_shutdown>([executor] {
            executor->loop_for(100, seconds(10));
        });

        const auto now = high_resolution_clock::now();

        assert_bigger_equal(now, shutdown_time);
        assert_smaller_equal(now, shutdown_time + seconds(1));

        thread.join();
    }
}

void concurrencpp::tests::test_manual_executor_loop_until() {
    // when max_count == 0, the function returns immediately
    {
        object_observer observer;
        auto executor = std::make_shared<concurrencpp::manual_executor>();
        executor_shutdowner shutdown(executor);

        const auto max_waiting_time_point = high_resolution_clock::now() + seconds(10);
        const auto before = high_resolution_clock::now();
        const auto executed = executor->loop_until(0, max_waiting_time_point);
        const auto after = high_resolution_clock::now();
        const auto ms_elapsed = duration_cast<milliseconds>(after - before);

        assert_equal(executed, static_cast<size_t>(0));
        assert_smaller_equal(ms_elapsed, milliseconds(5));
    }

    // when deadline <= now, the function returns 0
    {
        object_observer observer;
        auto executor = std::make_shared<concurrencpp::manual_executor>();
        executor_shutdowner shutdown(executor);

        const auto now = high_resolution_clock::now();
        std::this_thread::sleep_for(milliseconds(1));

        assert_equal(executor->loop_until(100, now), static_cast<size_t>(0));

        executor->post(observer.get_testing_stub());

        assert_equal(executor->loop_until(100, now), static_cast<size_t>(0));

        assert_equal(observer.get_execution_count(), static_cast<size_t>(0));
        assert_equal(observer.get_destruction_count(), static_cast<size_t>(0));
    }

    // if max_count is reached, the function returns
    {
        object_observer observer;
        auto executor = std::make_shared<concurrencpp::manual_executor>();
        executor_shutdowner shutdown(executor);

        const auto max_looping_time_point = high_resolution_clock::now() + seconds(10);
        const auto enqueueing_interval = milliseconds(100);
        const size_t max_count = 8;

        std::thread enqueuer([max_count, enqueueing_interval, executor, &observer] {
            for (size_t i = 0; i < max_count + 1; i++) {
                std::this_thread::sleep_for(enqueueing_interval);
                executor->post(observer.get_testing_stub());
            }
        });

        const auto executed = executor->loop_until(max_count, max_looping_time_point);

        assert_equal(executed, max_count);
        assert_smaller(high_resolution_clock::now(), max_looping_time_point);

        enqueuer.join();
    }

    // if shutdown requested, the function returns and throws
    {
        auto executor = std::make_shared<concurrencpp::manual_executor>();
        executor_shutdowner shutdown(executor);
        const auto shutdown_time = high_resolution_clock::now() + milliseconds(150);

        std::thread thread([executor, shutdown_time]() mutable {
            std::this_thread::sleep_until(shutdown_time);
            executor->shutdown();
        });

        assert_throws<errors::runtime_shutdown>([executor] {
            executor->loop_until(100, high_resolution_clock::now() + seconds(10));
        });

        const auto now = high_resolution_clock::now();

        assert_bigger_equal(now, shutdown_time);
        assert_smaller_equal(now, shutdown_time + seconds(1));

        thread.join();
    }
}

void concurrencpp::tests::test_manual_executor_clear() {
    object_observer observer;
    const size_t task_count = 100;
    auto executor = std::make_shared<concurrencpp::manual_executor>();
    executor_shutdowner shutdown(executor);

    assert_equal(executor->clear(), static_cast<size_t>(0));

    std::vector<result<size_t>> results;
    results.resize(task_count);

    for (size_t i = 0; i < task_count; i++) {
        results[i] = executor->submit(observer.get_testing_stub(i));
    }

    assert_equal(executor->clear(), task_count);
    assert_true(executor->empty());
    assert_equal(executor->size(), static_cast<size_t>(0));
    assert_equal(observer.get_execution_count(), static_cast<size_t>(0));

    for (auto& result : results) {
        assert_throws<concurrencpp::errors::broken_task>([&result]() mutable {
            result.get();
        });
    }

    assert_equal(observer.get_destruction_count(), task_count);

    assert_equal(executor->clear(), static_cast<size_t>(0));
}

void concurrencpp::tests::test_manual_executor_wait_for_task() {
    // case 1: tasks already exist
    {
        auto executor = std::make_shared<concurrencpp::manual_executor>();
        executor_shutdowner shutdown(executor);

        executor->post([] {
        });

        const auto before = high_resolution_clock::now();
        executor->wait_for_task();
        const auto after = high_resolution_clock::now();
        const auto ms_elapsed = std::chrono::duration_cast<milliseconds>(after - before).count();
        assert_smaller_equal(ms_elapsed, 5);
    }

    // case 2: goes to sleep, woken by a task
    {
        auto executor = std::make_shared<concurrencpp::manual_executor>();
        executor_shutdowner shutdown(executor);
        auto enqueuing_time = high_resolution_clock::now() + milliseconds(150);

        std::thread enqueuing_thread([executor, enqueuing_time]() mutable {
            std::this_thread::sleep_until(enqueuing_time);
            executor->post([] {
            });
        });

        assert_equal(executor->size(), static_cast<size_t>(0));
        executor->wait_for_task();
        const auto now = high_resolution_clock::now();
        assert_equal(executor->size(), static_cast<size_t>(1));

        assert_bigger_equal(high_resolution_clock::now(), enqueuing_time);

        enqueuing_thread.join();
    }

    // case 3: goes to sleep, wakes up by an interrupt
    {
        auto executor = std::make_shared<concurrencpp::manual_executor>();
        executor_shutdowner shutdown(executor);
        const auto shutdown_time = high_resolution_clock::now() + milliseconds(150);

        std::thread thread([executor, shutdown_time]() mutable {
            std::this_thread::sleep_until(shutdown_time);
            executor->shutdown();
        });

        assert_throws<errors::runtime_shutdown>([executor] {
            executor->wait_for_task();
        });

        const auto now = high_resolution_clock::now();

        assert_bigger_equal(now, shutdown_time);
        assert_smaller_equal(now, shutdown_time + seconds(1));

        thread.join();
    }
}

void concurrencpp::tests::test_manual_executor_wait_for_task_for() {
    // case 1: timeout
    {
        auto executor = std::make_shared<concurrencpp::manual_executor>();
        const auto waiting_time_ms = milliseconds(50);
        executor_shutdowner shutdown(executor);

        for (size_t i = 0; i < 10; i++) {
            const auto before = high_resolution_clock::now();
            assert_false(executor->wait_for_task_for(waiting_time_ms));
            const auto after = high_resolution_clock::now();
            const auto ms_elapsed = std::chrono::duration_cast<milliseconds>(after - before);
            assert_equal(executor->size(), 0);
            assert_bigger_equal(ms_elapsed, waiting_time_ms);
        }
    }

    // case 2: tasks already exist
    {
        auto executor = std::make_shared<concurrencpp::manual_executor>();
        const auto waiting_time_ms = milliseconds(150);
        executor_shutdowner shutdown(executor);

        executor->post([] {
        });

        const auto before = high_resolution_clock::now();
        assert_true(executor->wait_for_task_for(waiting_time_ms));
        const auto after = high_resolution_clock::now();
        const auto ms_elapsed = std::chrono::duration_cast<milliseconds>(after - before).count();
        assert_smaller_equal(ms_elapsed, 5);
    }

    // case 3: goes to sleep, then woken by an incoming task
    {
        auto executor = std::make_shared<concurrencpp::manual_executor>();
        executor_shutdowner shutdown(executor);

        const auto enqueuing_time = high_resolution_clock::now() + milliseconds(150);

        std::thread thread([executor, enqueuing_time]() mutable {
            std::this_thread::sleep_until(enqueuing_time);
            executor->post([] {
            });
        });

        assert_true(executor->wait_for_task_for(seconds(10)));
        const auto now = high_resolution_clock::now();

        assert_equal(executor->size(), static_cast<size_t>(1));
        assert_bigger_equal(now, enqueuing_time);
        assert_smaller_equal(now, enqueuing_time + seconds(1));

        thread.join();
    }

    // case 4: goes to sleep, then woken by an interrupt
    {
        auto executor = std::make_shared<concurrencpp::manual_executor>();
        executor_shutdowner shutdown(executor);
        const auto shutdown_time = high_resolution_clock::now() + milliseconds(150);

        std::thread thread([executor, shutdown_time]() mutable {
            std::this_thread::sleep_until(shutdown_time);
            executor->shutdown();
        });

        assert_throws<errors::runtime_shutdown>([executor] {
            executor->wait_for_task_for(seconds(10));
        });

        const auto now = high_resolution_clock::now();

        assert_bigger_equal(now, shutdown_time);
        assert_smaller_equal(now, shutdown_time + seconds(1));

        thread.join();
    }
}

void concurrencpp::tests::test_manual_executor_wait_for_task_until() {
    // case 1: timeout
    {
        auto executor = std::make_shared<concurrencpp::manual_executor>();
        executor_shutdowner shutdown(executor);

        for (size_t i = 0; i < 10; i++) {
            const auto max_waiting_time_point = high_resolution_clock::now() + milliseconds(50);
            assert_false(executor->wait_for_task_until(max_waiting_time_point));
            const auto now = high_resolution_clock::now();
            assert_bigger_equal(now, max_waiting_time_point);
        }
    }

    // case 2: tasks already exist
    {
        auto executor = std::make_shared<concurrencpp::manual_executor>();
        executor_shutdowner shutdown(executor);

        const auto max_waiting_time_point = high_resolution_clock::now() + milliseconds(150);

        executor->post([] {
        });

        const auto before = high_resolution_clock::now();
        assert_true(executor->wait_for_task_until(max_waiting_time_point));
        const auto after = high_resolution_clock::now();
        const auto ms_elapsed = std::chrono::duration_cast<milliseconds>(after - before).count();
        assert_smaller_equal(ms_elapsed, 5);
    }

    // case 3: goes to sleep, then woken by an incoming task
    {
        auto executor = std::make_shared<concurrencpp::manual_executor>();
        executor_shutdowner shutdown(executor);
        const auto enqueue_time = high_resolution_clock::now() + milliseconds(150);

        std::thread thread([executor, enqueue_time]() mutable {
            std::this_thread::sleep_until(enqueue_time);
            executor->post([] {
            });
        });

        assert_true(executor->wait_for_task_until(high_resolution_clock::now() + seconds(10)));
        const auto now = high_resolution_clock::now();

        assert_bigger_equal(now, enqueue_time);
        assert_smaller_equal(now, enqueue_time + seconds(1));

        thread.join();
    }

    // case 4: goes to sleep, then woken by an interrupt
    {
        auto executor = std::make_shared<concurrencpp::manual_executor>();
        executor_shutdowner shutdown(executor);
        const auto shutdown_time = high_resolution_clock::now() + milliseconds(150);

        std::thread thread([executor, shutdown_time]() mutable {
            std::this_thread::sleep_until(shutdown_time);
            executor->shutdown();
        });

        assert_throws<errors::runtime_shutdown>([executor] {
            executor->wait_for_task_until(high_resolution_clock::now() + seconds(10));
        });

        const auto now = high_resolution_clock::now();

        assert_bigger_equal(now, shutdown_time);
        assert_smaller_equal(now, shutdown_time + seconds(1));

        thread.join();
    }
}

void concurrencpp::tests::test_manual_executor_wait_for_tasks() {
    constexpr size_t task_count = 4;

    // case 0: max_count == 0, the function returns immediately
    {
        auto executor = std::make_shared<concurrencpp::manual_executor>();
        executor_shutdowner shutdown(executor);

        const auto before = high_resolution_clock::now();
        executor->wait_for_tasks(0);
        const auto after = high_resolution_clock::now();
        const auto ms_elapsed = std::chrono::duration_cast<milliseconds>(after - before).count();
        assert_smaller_equal(ms_elapsed, 5);
    }

    // case 1: max_count tasks already exist
    {
        auto executor = std::make_shared<concurrencpp::manual_executor>();
        executor_shutdowner shutdown(executor);

        for (size_t i = 0; i < task_count; i++) {
            executor->post([] {
            });
        }

        const auto before = high_resolution_clock::now();
        executor->wait_for_tasks(4);
        const auto after = high_resolution_clock::now();
        const auto ms_elapsed = std::chrono::duration_cast<milliseconds>(after - before).count();
        assert_smaller_equal(ms_elapsed, 5);
    }

    // case 2: goes to sleep, woken by incoming tasks
    {
        auto executor = std::make_shared<concurrencpp::manual_executor>();
        executor_shutdowner shutdown(executor);
        const auto enqueuing_interval = milliseconds(100);
        const auto before = high_resolution_clock::now();
        std::thread enqueuing_thread([executor, enqueuing_interval]() mutable {
            for (size_t i = 0; i < task_count; i++) {
                std::this_thread::sleep_for(enqueuing_interval);
                executor->post([] {
                });
            }
        });

        executor->wait_for_tasks(task_count);
        const auto now = high_resolution_clock::now();

        assert_equal(executor->size(), task_count);
        assert_bigger_equal(now, before + enqueuing_interval * task_count);
        assert_smaller_equal(now, before + enqueuing_interval * task_count + seconds(1));

        enqueuing_thread.join();
    }

    // case 3: goes to sleep, wakes up by an interrupt
    {
        auto executor = std::make_shared<concurrencpp::manual_executor>();
        executor_shutdowner shutdown(executor);
        const auto shutdown_time = high_resolution_clock::now() + milliseconds(150);

        std::thread thread([executor, shutdown_time]() mutable {
            std::this_thread::sleep_until(shutdown_time);
            executor->shutdown();
        });

        assert_throws<errors::runtime_shutdown>([executor] {
            executor->wait_for_tasks(task_count);
        });

        const auto now = high_resolution_clock::now();

        assert_bigger_equal(now, shutdown_time);
        assert_smaller_equal(now, shutdown_time + seconds(1));

        thread.join();
    }
}

void concurrencpp::tests::test_manual_executor_wait_for_tasks_for() {
    constexpr size_t task_count = 4;

    // case 0: max_count == 0, the function returns immediately
    {
        auto executor = std::make_shared<concurrencpp::manual_executor>();
        executor_shutdowner shutdown(executor);

        const auto before = high_resolution_clock::now();
        executor->wait_for_tasks_for(0, seconds(4));
        const auto after = high_resolution_clock::now();
        const auto ms_elapsed = std::chrono::duration_cast<milliseconds>(after - before).count();
        assert_smaller_equal(ms_elapsed, 5);
    }

    // case 1: timeout
    {
        auto executor = std::make_shared<concurrencpp::manual_executor>();
        const auto waiting_time_ms = milliseconds(50);
        executor_shutdowner shutdown(executor);

        for (size_t i = 0; i < 10; i++) {
            const auto before = high_resolution_clock::now();
            assert_false(executor->wait_for_tasks_for(task_count, waiting_time_ms));
            const auto after = high_resolution_clock::now();
            const auto ms_elapsed = std::chrono::duration_cast<milliseconds>(after - before);
            assert_equal(executor->size(), 0);
            assert_bigger_equal(ms_elapsed, waiting_time_ms);
        }
    }

    // case 2: max_count tasks already exist
    {
        auto executor = std::make_shared<concurrencpp::manual_executor>();
        const auto waiting_time_ms = milliseconds(150);
        executor_shutdowner shutdown(executor);

        for (size_t i = 0; i < task_count; i++) {
            executor->post([] {
            });
        }

        const auto before = high_resolution_clock::now();
        assert_true(executor->wait_for_tasks_for(task_count, waiting_time_ms));
        const auto after = high_resolution_clock::now();
        const auto ms_elapsed = std::chrono::duration_cast<milliseconds>(after - before).count();
        assert_smaller_equal(ms_elapsed, 5);
    }

    // case 3: goes to sleep, then woken by incoming tasks
    {
        auto executor = std::make_shared<concurrencpp::manual_executor>();
        executor_shutdowner shutdown(executor);
        const auto enqueuing_interval = milliseconds(100);
        const auto before = high_resolution_clock::now();

        std::thread enqueuing_thread([executor, enqueuing_interval]() mutable {
            for (size_t i = 0; i < task_count; i++) {
                std::this_thread::sleep_for(enqueuing_interval);
                executor->post([] {
                });
            }
        });

        executor->wait_for_tasks_for(task_count, std::chrono::seconds(10));

        const auto now = high_resolution_clock::now();

        assert_equal(executor->size(), task_count);
        assert_bigger_equal(now, before + enqueuing_interval * task_count);
        assert_smaller_equal(now, before + enqueuing_interval * task_count + seconds(1));

        enqueuing_thread.join();
    }

    // case 4: goes to sleep, then woken by an interrupt
    {
        auto executor = std::make_shared<concurrencpp::manual_executor>();
        executor_shutdowner shutdown(executor);
        const auto shutdown_time = high_resolution_clock::now() + milliseconds(150);

        std::thread thread([executor, shutdown_time]() mutable {
            std::this_thread::sleep_until(shutdown_time);
            executor->shutdown();
        });

        assert_throws<errors::runtime_shutdown>([executor] {
            executor->wait_for_tasks_for(task_count, seconds(10));
        });

        const auto now = high_resolution_clock::now();

        assert_bigger_equal(now, shutdown_time);
        assert_smaller_equal(now, shutdown_time + seconds(1));

        thread.join();
    }
}

void concurrencpp::tests::test_manual_executor_wait_for_tasks_until() {
    constexpr size_t task_count = 4;

    // case 0: max_count == 0, the function returns immediately
    {
        auto executor = std::make_shared<concurrencpp::manual_executor>();
        executor_shutdowner shutdown(executor);

        const auto before = high_resolution_clock::now();
        executor->wait_for_tasks_until(0, high_resolution_clock::now() + seconds(4));
        const auto after = high_resolution_clock::now();
        const auto ms_elapsed = std::chrono::duration_cast<milliseconds>(after - before).count();
        assert_smaller_equal(ms_elapsed, 5);
    }

    // case 1: timeout
    {
        auto executor = std::make_shared<concurrencpp::manual_executor>();
        executor_shutdowner shutdown(executor);

        for (size_t i = 0; i < 10; i++) {
            const auto max_waiting_time_point = high_resolution_clock::now() + milliseconds(50);
            assert_false(executor->wait_for_tasks_until(task_count, max_waiting_time_point));
            const auto after = high_resolution_clock::now();
            assert_equal(executor->size(), 0);
            assert_bigger_equal(after, max_waiting_time_point);
        }
    }

    // case 2: max_count tasks already exist
    {
        auto executor = std::make_shared<concurrencpp::manual_executor>();
        executor_shutdowner shutdown(executor);
        const auto max_waiting_time_point = high_resolution_clock::now() + milliseconds(150);

        for (size_t i = 0; i < task_count; i++) {
            executor->post([] {
            });
        }

        const auto before = high_resolution_clock::now();
        assert_true(executor->wait_for_tasks_until(task_count, max_waiting_time_point));
        const auto after = high_resolution_clock::now();
        const auto ms_elapsed = std::chrono::duration_cast<milliseconds>(after - before).count();
        assert_smaller_equal(ms_elapsed, 5);
    }

    // case 3: goes to sleep, then woken by incoming tasks
    {
        auto executor = std::make_shared<concurrencpp::manual_executor>();
        executor_shutdowner shutdown(executor);
        const auto enqueuing_interval = milliseconds(150);

        const auto before = high_resolution_clock::now();

        std::thread enqueuing_thread([executor, enqueuing_interval]() mutable {
            for (size_t i = 0; i < task_count; i++) {
                std::this_thread::sleep_for(enqueuing_interval);
                executor->post([] {
                });
            }
        });

        executor->wait_for_tasks_until(task_count, high_resolution_clock::now() + std::chrono::seconds(10));

        const auto now = high_resolution_clock::now();

        assert_equal(executor->size(), task_count);
        assert_bigger_equal(now, before + enqueuing_interval * task_count);
        assert_smaller_equal(now, before + enqueuing_interval * task_count + seconds(2));

        enqueuing_thread.join();
    }

    // case 4: goes to sleep, then woken by an interrupt
    {
        auto executor = std::make_shared<concurrencpp::manual_executor>();
        executor_shutdowner shutdown(executor);
        const auto shutdown_time = high_resolution_clock::now() + milliseconds(150);

        std::thread thread([executor, shutdown_time]() mutable {
            std::this_thread::sleep_until(shutdown_time);
            executor->shutdown();
        });

        assert_throws<errors::runtime_shutdown>([executor] {
            executor->wait_for_tasks_until(task_count, high_resolution_clock::now() + seconds(10));
        });

        const auto now = high_resolution_clock::now();

        assert_bigger_equal(now, shutdown_time);
        assert_smaller_equal(now, shutdown_time + seconds(1));

        thread.join();
    }
}

using namespace concurrencpp::tests;

int main() {
    tester tester("manual_executor test");

    tester.add_step("name", test_manual_executor_name);
    tester.add_step("shutdown", test_manual_executor_shutdown);
    tester.add_step("max_concurrency_level", test_manual_executor_max_concurrency_level);
    tester.add_step("post", test_manual_executor_post);
    tester.add_step("submit", test_manual_executor_submit);
    tester.add_step("bulk_post", test_manual_executor_bulk_post);
    tester.add_step("bulk_submit", test_manual_executor_bulk_submit);
    tester.add_step("loop_once", test_manual_executor_loop_once);
    tester.add_step("loop_once_for", test_manual_executor_loop_once_for);
    tester.add_step("loop_once_until", test_manual_executor_loop_once_until);
    tester.add_step("loop", test_manual_executor_loop);
    tester.add_step("loop_for", test_manual_executor_loop_for);
    tester.add_step("loop_until", test_manual_executor_loop_until);
    tester.add_step("wait_for_task", test_manual_executor_wait_for_task);
    tester.add_step("wait_for_task_for", test_manual_executor_wait_for_task_for);
    tester.add_step("wait_for_task_until", test_manual_executor_wait_for_task_until);
    tester.add_step("wait_for_tasks", test_manual_executor_wait_for_tasks);
    tester.add_step("wait_for_tasks_for", test_manual_executor_wait_for_tasks_for);
    tester.add_step("wait_for_tasks_until", test_manual_executor_wait_for_tasks_until);
    tester.add_step("clear", test_manual_executor_clear);

    tester.launch_test();
    return 0;
}