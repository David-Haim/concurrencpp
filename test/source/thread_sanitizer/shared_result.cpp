#include "concurrencpp/concurrencpp.h"

#include <iostream>

void shared_result_get(std::shared_ptr<concurrencpp::thread_executor> tp);
void shared_result_wait(std::shared_ptr<concurrencpp::thread_executor> tp);
void shared_result_wait_for(std::shared_ptr<concurrencpp::thread_executor> tp);
concurrencpp::result<void> shared_result_await(std::shared_ptr<concurrencpp::thread_executor> tp);
concurrencpp::result<void> shared_result_await_via(std::shared_ptr<concurrencpp::thread_executor> tp);

int main() {
    concurrencpp::runtime runtime;

    std::cout << "shared_result::get" << std::endl;
    shared_result_get(runtime.thread_executor());
    std::cout << "================================" << std::endl;

    std::cout << "shared_result::wait" << std::endl;
    shared_result_wait(runtime.thread_executor());
    std::cout << "================================" << std::endl;

    std::cout << "shared_result::wait_for" << std::endl;
    shared_result_wait_for(runtime.thread_executor());
    std::cout << "================================" << std::endl;

    std::cout << "shared_result::await" << std::endl;
    shared_result_await(runtime.thread_executor()).get();
    std::cout << "================================" << std::endl;

    std::cout << "shared_result::await_via" << std::endl;
    shared_result_await_via(runtime.thread_executor()).get();
    std::cout << "================================" << std::endl;
}

using namespace concurrencpp;

void shared_result_get(std::shared_ptr<thread_executor> tp) {
    const size_t task_count = 512;

    std::vector<shared_result<int>> results;
    results.reserve(task_count);

    for (size_t i = 0; i < task_count; i++) {
        results.emplace_back(tp->submit([i] {
            std::this_thread::yield();
            return int(i);
        }));
    }

    for (size_t i = 0; i < task_count; i++) {
        auto res = results[i].get();
        if (res != i) {
            std::cerr << "submit + get, expected " << i << " and got " << res << std::endl;
            std::abort();
        }
    }
}

void shared_result_wait(std::shared_ptr<thread_executor> tp) {
    const size_t task_count = 512;

    std::vector<shared_result<int>> results;
    results.reserve(task_count);

    for (size_t i = 0; i < task_count; i++) {
        results.emplace_back(tp->submit([i] {
            std::this_thread::yield();
            return int(i);
        }));
    }

    for (size_t i = 0; i < task_count; i++) {
        results[i].wait();
        auto res = results[i].get();
        if (res != i) {
            std::cerr << "submit + get, expected " << i << " and got " << res << std::endl;
            std::abort();
        }
    }
}

void shared_result_wait_for(std::shared_ptr<thread_executor> tp) {
    const size_t task_count = 512;

    std::vector<shared_result<int>> results;
    results.reserve(task_count);

    for (size_t i = 0; i < task_count; i++) {
        results.emplace_back(tp->submit([i] {
            std::this_thread::yield();
            return int(i);
        }));
    }

    for (size_t i = 0; i < task_count; i++) {
        while (results[i].wait_for(std::chrono::milliseconds(5)) == result_status::idle)
            ;

        auto res = results[i].get();
        if (res != i) {
            std::cerr << "submit + get, expected " << i << " and got " << res << std::endl;
            std::abort();
        }
    }
}

result<void> shared_result_await(std::shared_ptr<thread_executor> tp) {
    const size_t task_count = 512;

    std::vector<shared_result<int>> results;
    results.reserve(task_count);

    for (size_t i = 0; i < task_count; i++) {
        results.emplace_back(tp->submit([i] {
            std::this_thread::yield();
            return int(i);
        }));
    }

    for (size_t i = 0; i < task_count; i++) {
        auto res = co_await results[i];
        if (res != i) {
            std::cerr << "submit + await, expected " << i << " and got " << res << std::endl;
            std::abort();
        }
    }
}

result<void> shared_result_await_via(std::shared_ptr<thread_executor> tp) {
    const size_t task_count = 512;

    std::vector<shared_result<int>> results;
    results.reserve(task_count);

    for (size_t i = 0; i < task_count; i++) {
        results.emplace_back(tp->submit([i] {
            std::this_thread::yield();
            return int(i);
        }));
    }

    for (size_t i = 0; i < task_count; i++) {
        auto res = co_await results[i].await_via(tp);
        if (res != i) {
            std::cerr << "submit + await, expected " << i << " and got " << res << std::endl;
            std::abort();
        }
    }
}
