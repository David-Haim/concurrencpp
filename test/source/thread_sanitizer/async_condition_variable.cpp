#include "concurrencpp/concurrencpp.h"

#include <chrono>
#include <algorithm>
#include <iostream>

void test_async_cv_await();
void test_async_cv_await_pred();
void test_async_cv_notify_one();
void test_async_cv_notify_all();

int main() {
    std::cout << "Starting async_condition_variable test" << std::endl;
    std::cout << "================================" << std::endl;

    std::cout << "async_condition_variable::await test" << std::endl;
    test_async_cv_await();
    std::cout << "================================" << std::endl;

    std::cout << "async_condition_variable::await(pred) test" << std::endl;
    test_async_cv_await_pred();
    std::cout << "================================" << std::endl;

    std::cout << "async_condition_variable::notify_one test" << std::endl;
    test_async_cv_notify_one();
    std::cout << "================================" << std::endl;

    std::cout << "async_condition_variable::notify_all test" << std::endl;
    test_async_cv_notify_all();
    std::cout << "================================" << std::endl;

    std::cout << "done" << std::endl;
    std::cout << "================================" << std::endl;
}

using namespace concurrencpp;
using namespace std::chrono;

void test_async_cv_await() {
    concurrencpp::runtime runtime;

    async_lock lock;
    async_condition_variable cv;

    constexpr size_t task_count = 512;
    const auto deadline = system_clock::now() + seconds(5);

    std::vector<result<void>> results;
    results.reserve(task_count);

    auto task = [&](executor_tag, std::shared_ptr<worker_thread_executor> te) -> result<void> {
        std::this_thread::sleep_until(deadline);

        auto guard = co_await lock.lock(te);
        co_await cv.await(te, guard);
    };

    for (size_t i = 0; i < task_count; i++) {
        auto wte = runtime.make_worker_thread_executor();
        results.emplace_back(task({}, wte));
    }

    std::this_thread::sleep_until(deadline + std::chrono::seconds(5));

    cv.notify_all();

    for (auto& result : results) {
        result.get();
    }
}

void test_async_cv_await_pred() {
    concurrencpp::runtime runtime;

    int pred = 10;
    async_lock lock;
    async_condition_variable cv;

    constexpr size_t task_count = 512;
    const auto deadline = system_clock::now() + seconds(5);

    std::vector<result<void>> results;
    results.reserve(task_count);

    auto task = [&](executor_tag, std::shared_ptr<worker_thread_executor> te) -> result<void> {
        std::this_thread::sleep_until(deadline);

        auto guard = co_await lock.lock(te);
        co_await cv.await(te, guard, [&] {
            return pred < 10;
        });
    };

    for (size_t i = 0; i < task_count; i++) {
        auto wte = runtime.make_worker_thread_executor();
        results.emplace_back(task({}, wte));
    }

    std::this_thread::sleep_until(deadline + std::chrono::seconds(5));

    cv.notify_all();

    std::this_thread::sleep_for(std::chrono::seconds(5));

    [&](executor_tag, std::shared_ptr<concurrencpp::thread_executor> te) -> result<void> {
        auto guard = co_await lock.lock(te);
        pred = -10;
        guard.unlock();
    }({}, runtime.thread_executor())
                                                                                .get();

    cv.notify_all();

    for (auto& result : results) {
        result.get();
    }
}

void test_async_cv_notify_one() {
    concurrencpp::runtime runtime;

    async_lock lock;
    async_condition_variable cv;

    constexpr size_t task_count = 512;
    const auto deadline = system_clock::now() + seconds(5);

    std::vector<result<void>> results;
    results.reserve(task_count * 2);

    auto waiter_task = [&](executor_tag, std::shared_ptr<worker_thread_executor> te) -> result<void> {
        std::this_thread::sleep_until(deadline);

        auto guard = co_await lock.lock(te);
        co_await cv.await(te, guard);
    };

    for (size_t i = 0; i < task_count; i++) {
        auto wte = runtime.make_worker_thread_executor();
        results.emplace_back(waiter_task({}, wte));
    }

    auto notifier_task = [&](executor_tag, std::shared_ptr<thread_executor> te) -> result<void> {
        std::this_thread::sleep_until(deadline + milliseconds(1));
        cv.notify_one();
        co_return;
    };

    for (size_t i = 0; i < task_count; i++) {
        results.emplace_back(notifier_task({}, runtime.thread_executor()));
    }

    for (auto& result : results) {
        result.get();
    }
}

void test_async_cv_notify_all() {
    concurrencpp::runtime runtime;

    async_lock lock;
    async_condition_variable cv;

    constexpr size_t task_count = 512;
    const auto deadline = system_clock::now() + seconds(5);

    std::vector<result<void>> results;
    results.reserve(task_count * 2);

    auto waiter_task = [&](executor_tag, std::shared_ptr<worker_thread_executor> te) -> result<void> {
        std::this_thread::sleep_until(deadline);

        auto guard = co_await lock.lock(te);
        co_await cv.await(te, guard);
    };

    for (size_t i = 0; i < task_count; i++) {
        auto wte = runtime.make_worker_thread_executor();
        results.emplace_back(waiter_task({}, wte));
    }

    auto notifier_task = [&](executor_tag, std::shared_ptr<thread_executor> te) -> result<void> {
        std::this_thread::sleep_until(deadline + milliseconds(1));
        cv.notify_all();
        co_return;
    };

    for (size_t i = 0; i < task_count; i++) {
        results.emplace_back(notifier_task({}, runtime.thread_executor()));
    }

    for (auto& result : results) {
        result.get();
    }
}
