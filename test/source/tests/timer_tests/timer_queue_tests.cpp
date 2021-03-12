#include "concurrencpp/concurrencpp.h"

#include "infra/tester.h"
#include "infra/assertions.h"
#include "utils/object_observer.h"
#include "utils/executor_shutdowner.h"

#include <chrono>

using namespace std::chrono_literals;

namespace concurrencpp::tests {
    void test_timer_queue_make_timer();
    void test_timer_queue_make_oneshot_timer();
    void test_timer_queue_make_delay_object();
    void test_timer_queue_max_worker_idle_time();
    void test_timer_queue_thread_injection();
}  // namespace concurrencpp::tests

void concurrencpp::tests::test_timer_queue_make_timer() {
    auto timer_queue = std::make_shared<concurrencpp::timer_queue>(120s);
    assert_false(timer_queue->shutdown_requested());

    assert_throws_with_error_message<std::invalid_argument>(
        [timer_queue] {
            timer_queue->make_timer(100ms, 100ms, {}, [] {
            });
        },
        concurrencpp::details::consts::k_timer_queue_make_timer_executor_null_err_msg);

    timer_queue->shutdown();
    assert_true(timer_queue->shutdown_requested());

    assert_throws_with_error_message<errors::runtime_shutdown>(
        [timer_queue] {
            auto inline_executor = std::make_shared<concurrencpp::inline_executor>();
            timer_queue->make_timer(100ms, 100ms, inline_executor, [] {
            });
        },
        concurrencpp::details::consts::k_timer_queue_shutdown_err_msg);
}

void concurrencpp::tests::test_timer_queue_make_oneshot_timer() {
    auto timer_queue = std::make_shared<concurrencpp::timer_queue>(120s);
    assert_false(timer_queue->shutdown_requested());

    assert_throws_with_error_message<std::invalid_argument>(
        [timer_queue] {
            timer_queue->make_one_shot_timer(100ms, {}, [] {
            });
        },
        concurrencpp::details::consts::k_timer_queue_make_oneshot_timer_executor_null_err_msg);

    timer_queue->shutdown();
    assert_true(timer_queue->shutdown_requested());

    assert_throws_with_error_message<errors::runtime_shutdown>(
        [timer_queue] {
            auto inline_executor = std::make_shared<concurrencpp::inline_executor>();
            timer_queue->make_one_shot_timer(100ms, inline_executor, [] {
            });
        },
        concurrencpp::details::consts::k_timer_queue_shutdown_err_msg);
}

void concurrencpp::tests::test_timer_queue_make_delay_object() {
    auto timer_queue = std::make_shared<concurrencpp::timer_queue>(120s);
    assert_false(timer_queue->shutdown_requested());

    assert_throws_with_error_message<std::invalid_argument>(
        [timer_queue] {
            timer_queue->make_delay_object(100ms, {});
        },
        concurrencpp::details::consts::k_timer_queue_make_delay_object_executor_null_err_msg);

    timer_queue->shutdown();
    assert_true(timer_queue->shutdown_requested());

    assert_throws_with_error_message<errors::runtime_shutdown>(
        [timer_queue] {
            auto inline_executor = std::make_shared<concurrencpp::inline_executor>();
            timer_queue->make_delay_object(100ms, inline_executor);
        },
        concurrencpp::details::consts::k_timer_queue_shutdown_err_msg);
}

void concurrencpp::tests::test_timer_queue_max_worker_idle_time() {
    auto timer_queue = std::make_shared<concurrencpp::timer_queue>(1234567ms);
    assert_equal(timer_queue->max_worker_idle_time(), 1234567ms);
}

void concurrencpp::tests::test_timer_queue_thread_injection() {
    auto timer_queue = std::make_shared<concurrencpp::timer_queue>(50ms);
    object_observer observer;
    auto inline_executor = std::make_shared<concurrencpp::inline_executor>();
    executor_shutdowner es(inline_executor);

    for (size_t i = 0; i < 5; i++) {
        auto timer = timer_queue->make_one_shot_timer(50ms, inline_executor, observer.get_testing_stub());
        std::this_thread::sleep_for(timer_queue->max_worker_idle_time() + 100ms);
    }

    const auto& execution_map = observer.get_execution_map();
    assert_equal(execution_map.size(), 5);
    for (const auto& count : execution_map) {
        assert_equal(count.second, 1);
    }
}

using namespace concurrencpp::tests;

int main() {
    tester test("timer_queue test");

    test.add_step("make_timer", test_timer_queue_make_timer);
    test.add_step("make_oneshot_timer", test_timer_queue_make_timer);
    test.add_step("make_delay_object", test_timer_queue_make_delay_object);
    test.add_step("max_worker_idle_time", test_timer_queue_max_worker_idle_time);
    test.add_step("thread injection", test_timer_queue_thread_injection);

    test.launch_test();
    return 0;
}