#include "concurrencpp/concurrencpp.h"

#include "infra/tester.h"
#include "infra/assertions.h"
#include "utils/executor_shutdowner.h"

#include "concurrencpp/threads/constants.h"

using namespace concurrencpp;

namespace concurrencpp::tests {
    void test_async_condition_variable_await_null_resume_executor();
    void test_async_condition_variable_await_unlocked_scoped_async_lock();
    void test_async_condition_variable_await();

    void test_async_condition_variable_await_pred_null_resume_executor();
    void test_async_condition_variable_await_pred_unlocked_scoped_async_lock();
    void test_async_condition_variable_await_pred();

    void test_async_condition_variable_notify_one();
    void test_async_condition_variable_notify_all();
}  // namespace concurrencpp::tests

using namespace concurrencpp::tests;

void tests::test_async_condition_variable_await_null_resume_executor() {
    async_lock lock;
    async_condition_variable cv;
    const auto executor = std::make_shared<inline_executor>();
    executor_shutdowner es(executor);

    auto scoped_lock = lock.lock(executor).run().get();

    assert_throws_with_error_message<std::invalid_argument>(
        [&] {
            cv.await({}, scoped_lock).run().get();
        },
        concurrencpp::details::consts::k_async_condition_variable_await_invalid_resume_executor_err_msg);
}

void tests::test_async_condition_variable_await_unlocked_scoped_async_lock() {
    async_condition_variable cv;
    const auto executor = std::make_shared<inline_executor>();
    executor_shutdowner es(executor);

    assert_throws_with_error_message<std::invalid_argument>(
        [&] {
            scoped_async_lock sal;
            cv.await(executor, sal).run().get();
        },
        concurrencpp::details::consts::k_async_condition_variable_await_lock_unlocked_err_msg);
}

void tests::test_async_condition_variable_await() {
    test_async_condition_variable_await_null_resume_executor();
    test_async_condition_variable_await_unlocked_scoped_async_lock();

    async_lock lock;
    async_condition_variable cv;
    const auto executor = std::make_shared<inline_executor>();
    executor_shutdowner es(executor);

    auto task = [&]() -> result<void> {
        auto sal = co_await lock.lock(executor);
        co_await cv.await(executor, sal);
    };

    auto res = task();

    for (size_t i = 0; i < 5; i++) {
        assert_equal(res.status(), result_status::idle);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    cv.notify_one();
    assert_equal(res.status(), result_status::value);
    res.get();
}

void tests::test_async_condition_variable_await_pred_null_resume_executor() {
    async_lock lock;
    async_condition_variable cv;
    const auto executor = std::make_shared<inline_executor>();
    executor_shutdowner es(executor);

    auto scoped_lock = lock.lock(executor).run().get();

    assert_throws_with_error_message<std::invalid_argument>(
        [&] {
            cv.await({}, scoped_lock, [] {
                return true;
            });
        },
        concurrencpp::details::consts::k_async_condition_variable_await_invalid_resume_executor_err_msg);
}

void tests::test_async_condition_variable_await_pred_unlocked_scoped_async_lock() {
    async_lock lock;
    async_condition_variable cv;
    const auto executor = std::make_shared<inline_executor>();
    executor_shutdowner es(executor);

    assert_throws_with_error_message<std::invalid_argument>(
        [&] {
            scoped_async_lock sal;
            cv.await(executor, sal, [] {
                return true;
            });
        },
        concurrencpp::details::consts::k_async_condition_variable_await_lock_unlocked_err_msg);
}

void tests::test_async_condition_variable_await_pred() {
    test_async_condition_variable_await_pred_null_resume_executor();
    test_async_condition_variable_await_pred_unlocked_scoped_async_lock();

    async_lock lock;
    async_condition_variable cv;
    auto running = true;
    const auto executor = std::make_shared<inline_executor>();
    executor_shutdowner es(executor);

    auto task = [&]() -> result<void> {
        auto sal = co_await lock.lock(executor);
        co_await cv.await(executor, sal, [&] {
            return !running;
        });
    };

    auto res = task();

    for (size_t i = 0; i < 5; i++) {
        assert_equal(res.status(), result_status::idle);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    cv.notify_one();

    for (size_t i = 0; i < 5; i++) {
        assert_equal(res.status(), result_status::idle);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    auto task0 = [&]() -> result<void> {
        auto sal = co_await lock.lock(executor);
        running = false;
    };

    task0().get();

    assert_equal(res.status(), result_status::idle);

    cv.notify_one();

    assert_equal(res.status(), result_status::value);
    res.get();
}

void tests::test_async_condition_variable_notify_one() {
    async_lock lock;
    async_condition_variable cv;
    const auto executor = std::make_shared<inline_executor>();
    executor_shutdowner es(executor);

    auto task = [&]() -> result<void> {
        auto sal = co_await lock.lock(executor);
        co_await cv.await(executor, sal);
    };

    result<void> results[5];
    for (auto& result : results) {
        result = task();
    }

    for (auto& result : results) {
        assert_equal(result.status(), result_status::idle);
    }

    cv.notify_one();
    assert_equal(results[0].status(), result_status::value);
    for (size_t i = 1; i < std::size(results); i++) {
        assert_equal(results[i].status(), result_status::idle);
    }

    cv.notify_one();
    assert_equal(results[1].status(), result_status::value);
    for (size_t i = 2; i < std::size(results); i++) {
        assert_equal(results[i].status(), result_status::idle);
    }

    cv.notify_one();
    assert_equal(results[2].status(), result_status::value);
    for (size_t i = 3; i < std::size(results); i++) {
        assert_equal(results[i].status(), result_status::idle);
    }

    cv.notify_one();
    assert_equal(results[3].status(), result_status::value);
    for (size_t i = 4; i < std::size(results); i++) {
        assert_equal(results[i].status(), result_status::idle);
    }

    cv.notify_one();
    assert_equal(results[4].status(), result_status::value);
}

void tests::test_async_condition_variable_notify_all() {
    async_lock lock;
    async_condition_variable cv;
    const auto executor = std::make_shared<inline_executor>();
    executor_shutdowner es(executor);

    auto task = [&]() -> result<void> {
        auto sal = co_await lock.lock(executor);
        co_await cv.await(executor, sal);
    };

    result<void> results[5];
    for (auto& result : results) {
        result = task();
    }

    for (auto& result : results) {
        assert_equal(result.status(), result_status::idle);
    }

    cv.notify_all();
    for (auto& result : results) {
        assert_equal(result.status(), result_status::value);
    }
}

int main() {
    tester tester("async_condition_variable test");

    tester.add_step("await", test_async_condition_variable_await);
    tester.add_step("await + pred", test_async_condition_variable_await_pred);
    tester.add_step("notify_one", test_async_condition_variable_notify_one);
    tester.add_step("notify_all", test_async_condition_variable_notify_all);

    tester.launch_test();
    return 0;
}
