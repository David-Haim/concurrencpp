#include "concurrencpp/concurrencpp.h"

#include <iostream>

void test_worker_thread_executor();
void test_thread_pool_executor();
void test_thread_executor();
void test_manual_executor();

int main() {
    std::cout << "concurrencpp::worker_thread_executor" << std::endl;
    test_worker_thread_executor();
    std::cout << "====================================" << std::endl;

    std::cout << "concurrencpp::thread_pool_executor" << std::endl;
    test_thread_pool_executor();
    std::cout << "====================================" << std::endl;

    std::cout << "concurrencpp::thread_executor" << std::endl;
    test_thread_executor();
    std::cout << "====================================" << std::endl;

    std::cout << "concurrencpp::manual_executor" << std::endl;
    test_manual_executor();
    std::cout << "====================================" << std::endl;
}

using namespace concurrencpp;

void worker_thread_task(std::shared_ptr<worker_thread_executor> (&executors)[16],
                        std::atomic_size_t& counter,
                        std::shared_ptr<concurrencpp::details::wait_context> wc) {
    const auto c = counter.fetch_add(1, std::memory_order_relaxed);

    if (c >= 10'000'000) {
        if (c == 10'000'000) {
            wc->notify();
        }

        return;
    }

    const auto worker_pos = ::rand() % std::size(executors);
    auto& executor = executors[worker_pos];

    try {
        executor->post(worker_thread_task, std::ref(executors), std::ref(counter), wc);
    } catch (const concurrencpp::errors::executor_shutdown&) {
        return;
    }
}

void test_worker_thread_executor() {
    concurrencpp::runtime runtime;

    std::srand(::time(nullptr));
    std::shared_ptr<worker_thread_executor> executors[16];
    std::atomic_size_t counter = 0;
    auto wc = std::make_shared<concurrencpp::details::wait_context>();

    for (auto& executor : executors) {
        executor = runtime.make_worker_thread_executor();
    }

    for (size_t i = 0; i < 16; i++) {
        executors[i]->post(worker_thread_task, std::ref(executors), std::ref(counter), wc);
    }

    wc->wait();
}

void thread_pool_task(std::shared_ptr<thread_pool_executor> tpe, std::atomic_size_t& counter, std::shared_ptr<concurrencpp::details::wait_context> wc) {
    const auto c = counter.fetch_add(1, std::memory_order_relaxed);

    if (c >= 10'000'000) {
        if (c == 10'000'000) {
            wc->notify();
        }

        return;
    }

    try {
        tpe->post(thread_pool_task, tpe, std::ref(counter), wc);
    } catch (const concurrencpp::errors::executor_shutdown&) {
        return;
    }
}

void test_thread_pool_executor() {
    concurrencpp::runtime runtime;
    auto tpe = runtime.thread_pool_executor();
    std::atomic_size_t counter = 0;
    auto wc = std::make_shared<concurrencpp::details::wait_context>();
    const auto max_concurrency_level = tpe->max_concurrency_level();

    for (size_t i = 0; i < max_concurrency_level; i++) {
        tpe->post(thread_pool_task, tpe, std::ref(counter), wc);
    }

    wc->wait();
}

void thread_task(std::shared_ptr<thread_executor> tp, std::atomic_size_t& counter, std::shared_ptr<concurrencpp::details::wait_context> wc) {
    const auto c = counter.fetch_add(1, std::memory_order_relaxed);
    if (c >= 1'024 * 4) {
        if (c == 1'024 * 4) {
            wc->notify();
        }

        return;
    }

    try {
        tp->post(thread_task, tp, std::ref(counter), wc);
    } catch (const concurrencpp::errors::executor_shutdown&) {
        return;
    }
}

void test_thread_executor() {
    concurrencpp::runtime runtime;
    auto tp = runtime.thread_executor();
    std::atomic_size_t counter = 0;
    auto wc = std::make_shared<concurrencpp::details::wait_context>();

    for (size_t i = 0; i < 4; i++) {
        tp->post(thread_task, tp, std::ref(counter), wc);
    }

    wc->wait();
}

void manual_executor_work_loop(std::shared_ptr<manual_executor> (&executors)[16], std::atomic_size_t& counter, const size_t worker_index) {
    try {
        while (true) {
            const auto c = counter.fetch_add(1, std::memory_order_relaxed);

            if (c >= 10'000'000) {
                return;
            }

            const auto worker_pos = ::rand() % std::size(executors);
            auto& executor = executors[worker_pos];
            executor->post([] {
            });

            executors[worker_index]->loop(16);
        }
    } catch (const concurrencpp::errors::executor_shutdown&) {
        return;
    }
}

void test_manual_executor() {
    concurrencpp::runtime runtime;
    std::atomic_size_t counter = 0;
    std::shared_ptr<manual_executor> executors[16];
    std::thread threads[16];

    for (auto& executor : executors) {
        executor = runtime.make_manual_executor();
    }

    for (size_t i = 0; i < std::size(executors); i++) {
        threads[i] = std::thread([&, i] {
            manual_executor_work_loop(executors, std::ref(counter), i);
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }
}
