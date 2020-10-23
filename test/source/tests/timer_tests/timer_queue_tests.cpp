#include "concurrencpp/concurrencpp.h"
#include "tests/all_tests.h"

#include "tester/tester.h"
#include "helpers/assertions.h"
#include "helpers/object_observer.h"
#include "helpers/random.h"

#include <chrono>

using namespace std::chrono_literals;

namespace concurrencpp::tests {
void test_timer_queue_make_timer();
void test_timer_queue_make_oneshot_timer();
void test_timer_queue_make_delay_object();
}  // namespace concurrencpp::tests

void concurrencpp::tests::test_timer_queue_make_timer() {
    auto timer_queue = std::make_shared<concurrencpp::timer_queue>();
    assert_false(timer_queue->shutdown_requested());

    assert_throws_with_error_message<std::invalid_argument>(
        [timer_queue] {
            timer_queue->make_timer(100ms, 100ms, {}, [] {
            });
        },
        concurrencpp::details::consts::
            k_timer_queue_make_timer_executor_null_err_msg);

    timer_queue->shutdown();
    assert_true(timer_queue->shutdown_requested());

    assert_throws_with_error_message<errors::timer_queue_shutdown>(
        [timer_queue] {
            auto inline_executor =
                std::make_shared<concurrencpp::inline_executor>();
            timer_queue->make_timer(100ms, 100ms, inline_executor, [] {
            });
        },
        concurrencpp::details::consts::k_timer_queue_shutdown_err_msg);
}

void concurrencpp::tests::test_timer_queue_make_oneshot_timer() {
    auto timer_queue = std::make_shared<concurrencpp::timer_queue>();
    assert_false(timer_queue->shutdown_requested());

    assert_throws_with_error_message<std::invalid_argument>(
        [timer_queue] {
            timer_queue->make_one_shot_timer(100ms, {}, [] {
            });
        },
        concurrencpp::details::consts::
            k_timer_queue_make_oneshot_timer_executor_null_err_msg);

    timer_queue->shutdown();
    assert_true(timer_queue->shutdown_requested());

    assert_throws_with_error_message<errors::timer_queue_shutdown>(
        [timer_queue] {
            auto inline_executor =
                std::make_shared<concurrencpp::inline_executor>();
            timer_queue->make_one_shot_timer(100ms, inline_executor, [] {
            });
        },
        concurrencpp::details::consts::k_timer_queue_shutdown_err_msg);
}

void concurrencpp::tests::test_timer_queue_make_delay_object() {
    auto timer_queue = std::make_shared<concurrencpp::timer_queue>();
    assert_false(timer_queue->shutdown_requested());

    assert_throws_with_error_message<std::invalid_argument>(
        [timer_queue] {
            timer_queue->make_delay_object(100ms, {});
        },
        concurrencpp::details::consts::
            k_timer_queue_make_delay_object_executor_null_err_msg);

    timer_queue->shutdown();
    assert_true(timer_queue->shutdown_requested());

    assert_throws_with_error_message<errors::timer_queue_shutdown>(
        [timer_queue] {
            auto inline_executor =
                std::make_shared<concurrencpp::inline_executor>();
            timer_queue->make_delay_object(100ms, inline_executor);
        },
        concurrencpp::details::consts::k_timer_queue_shutdown_err_msg);
}

void concurrencpp::tests::test_timer_queue() {
    tester test("timer_queue test");

    test.add_step("make_timer", test_timer_queue_make_timer);
    test.add_step("make_oneshot_timer", test_timer_queue_make_timer);
    test.add_step("make_delay_object", test_timer_queue_make_delay_object);

    test.launch_test();
}
