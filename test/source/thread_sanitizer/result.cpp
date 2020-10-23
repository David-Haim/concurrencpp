#include "concurrencpp/concurrencpp.h"

#include <iostream>

void result_get(std::shared_ptr<concurrencpp::thread_pool_executor> tp);
void result_wait(std::shared_ptr<concurrencpp::thread_pool_executor> tp);
void result_wait_for(std::shared_ptr<concurrencpp::thread_pool_executor> tp);
concurrencpp::result<void> result_await(
    std::shared_ptr<concurrencpp::thread_pool_executor> tp);
concurrencpp::result<void> result_await_via(
    std::shared_ptr<concurrencpp::thread_pool_executor> tp);

int main() {
    concurrencpp::runtime_options opts;
    opts.max_cpu_threads = 24;
    concurrencpp::runtime runtime(opts);

    std::cout << "result::get" << std::endl;
    result_get(runtime.thread_pool_executor());
    std::cout << "================================" << std::endl;

    std::cout << "result::wait" << std::endl;
    result_wait(runtime.thread_pool_executor());
    std::cout << "================================" << std::endl;

    std::cout << "result::wait_for" << std::endl;
    result_wait_for(runtime.thread_pool_executor());
    std::cout << "================================" << std::endl;

    std::cout << "result::await" << std::endl;
    result_await(runtime.thread_pool_executor()).get();
    std::cout << "================================" << std::endl;

    std::cout << "result::await_via" << std::endl;
    result_await_via(runtime.thread_pool_executor()).get();
    std::cout << "================================" << std::endl;
}

using namespace concurrencpp;

void result_get(std::shared_ptr<thread_pool_executor> tp) {
    const size_t task_count = 8'000'000;

    std::vector<result<int>> results;
    results.reserve(task_count);

    for (size_t i = 0; i < task_count; i++) {
        results.emplace_back(tp->submit([i] {
            return int(i);
        }));
    }

    for (size_t i = 0; i < task_count; i++) {
        auto res = results[i].get();
        if (res != i) {
            std::cerr << "submit + get, expected " << i << " and got " << res
                      << std::endl;
            std::abort();
        }
    }
}

void result_wait(std::shared_ptr<thread_pool_executor> tp) {
    const size_t task_count = 8'000'000;

    std::vector<result<int>> results;
    results.reserve(task_count);

    for (size_t i = 0; i < task_count; i++) {
        results.emplace_back(tp->submit([i] {
            return int(i);
        }));
    }

    for (size_t i = 0; i < task_count; i++) {
        results[i].wait();
        auto res = results[i].get();
        if (res != i) {
            std::cerr << "submit + get, expected " << i << " and got " << res
                      << std::endl;
            std::abort();
        }
    }
}

void result_wait_for(std::shared_ptr<thread_pool_executor> tp) {
    const size_t task_count = 8'000'000;

    std::vector<result<int>> results;
    results.reserve(task_count);

    for (size_t i = 0; i < task_count; i++) {
        results.emplace_back(tp->submit([i] {
            return int(i);
        }));
    }

    for (size_t i = 0; i < task_count; i++) {
        while (results[i].wait_for(std::chrono::milliseconds(5)) ==
               result_status::idle)
            ;

        auto res = results[i].get();
        if (res != i) {
            std::cerr << "submit + get, expected " << i << " and got " << res
                      << std::endl;
            std::abort();
        }
    }
}

result<void> result_await(std::shared_ptr<thread_pool_executor> tp) {
    const size_t task_count = 8'000'000;

    std::vector<result<int>> results;
    results.reserve(task_count);

    for (size_t i = 0; i < task_count; i++) {
        results.emplace_back(tp->submit([i] {
            return int(i);
        }));
    }

    for (size_t i = 0; i < task_count; i++) {
        auto res = co_await results[i];
        if (res != i) {
            std::cerr << "submit + await, expected " << i << " and got " << res
                      << std::endl;
            std::abort();
        }
    }
}

result<void> result_await_via(std::shared_ptr<thread_pool_executor> tp) {
    const size_t task_count = 8'000'000;

    std::vector<result<int>> results;
    results.reserve(task_count);

    for (size_t i = 0; i < task_count; i++) {
        results.emplace_back(tp->submit([i] {
            return int(i);
        }));
    }

    for (size_t i = 0; i < task_count; i++) {
        auto res = co_await results[i].await_via(tp);
        if (res != i) {
            std::cerr << "submit + await, expected " << i << " and got " << res
                      << std::endl;
            std::abort();
        }
    }
}
