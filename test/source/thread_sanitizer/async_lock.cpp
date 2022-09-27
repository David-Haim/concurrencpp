#include "concurrencpp/concurrencpp.h"

#include <algorithm>
#include <iostream>

void async_increment(concurrencpp::runtime& runtime);
void async_insert(concurrencpp::runtime& runtime);

int main() {
    std::cout << "Starting async_lock test" << std::endl;

    concurrencpp::runtime runtime;

    std::cout << "================================" << std::endl;
    std::cout << "async increment" << std::endl;
    async_increment(runtime);

    std::cout << "================================" << std::endl;
    std::cout << "async insert" << std::endl;
    async_insert(runtime);

    std::cout << "================================" << std::endl;
}

using namespace concurrencpp;

result<void> incremenet(executor_tag,
                        std::shared_ptr<executor> ex,
                        async_lock& lock,
                        size_t& counter,
                        size_t cycles,
                        std::chrono::time_point<std::chrono::system_clock> tp) {

    std::this_thread::sleep_until(tp);

    for (size_t i = 0; i < cycles; i++) {
        auto lk = co_await lock.lock(ex);
        ++counter;
    }
}

result<void> insert(executor_tag,
                    std::shared_ptr<executor> ex,
                    async_lock& lock,
                    std::vector<size_t>& vec,
                    size_t range_begin,
                    size_t range_end,
                    std::chrono::time_point<std::chrono::system_clock> tp) {

    std::this_thread::sleep_until(tp);

    for (size_t i = range_begin; i < range_end; i++) {
        auto lk = co_await lock.lock(ex);
        vec.emplace_back(i);
    }
}

void async_increment(runtime& runtime) {
    async_lock mtx;
    size_t counter = 0;

    const size_t worker_count = concurrencpp::details::thread::hardware_concurrency();
    constexpr size_t cycles = 500'000;

    std::vector<std::shared_ptr<worker_thread_executor>> workers(worker_count);
    for (auto& worker : workers) {
        worker = runtime.make_worker_thread_executor();
    }

    std::vector<result<void>> results(worker_count);

    const auto deadline = std::chrono::system_clock::now() + std::chrono::seconds(4);

    for (size_t i = 0; i < worker_count; i++) {
        results[i] = incremenet({}, workers[i], mtx, counter, cycles, deadline);
    }

    for (size_t i = 0; i < worker_count; i++) {
        results[i].get();
    }

    {
        auto lock = mtx.lock(workers[0]).run().get();
        if (counter != cycles * worker_count) {
            std::cout << "async_lock test failed, counter != cycles * worker_count, " << counter << " " << cycles * worker_count
                      << std::endl;
        }
    }
}

void async_insert(runtime& runtime) {
    async_lock mtx;
    std::vector<size_t> vector;

    const size_t worker_count = concurrencpp::details::thread::hardware_concurrency();
    constexpr size_t cycles = 500'000;

    std::vector<std::shared_ptr<worker_thread_executor>> workers(worker_count);
    for (auto& worker : workers) {
        worker = runtime.make_worker_thread_executor();
    }

    std::vector<result<void>> results(worker_count);

    const auto deadline = std::chrono::system_clock::now() + std::chrono::seconds(4);

    for (size_t i = 0; i < worker_count; i++) {
        results[i] = insert({}, workers[i], mtx, vector, i * cycles, (i + 1) * cycles, deadline);
    }

    for (size_t i = 0; i < worker_count; i++) {
        results[i].get();
    }

    {
        auto lock = mtx.lock(workers[0]).run().get();
        if (vector.size() != cycles * worker_count) {
            std::cerr << "async_lock test failed, vector.size() != cycles * worker_count, " << vector.size()
                      << " != " << cycles * worker_count << std::endl;
        }

        std::sort(vector.begin(), vector.end());

        for (size_t i = 0; i < worker_count * cycles; i++) {
            if (vector[i] != i) {
                std::cerr << "async_lock test failed, vector[i] != i, " << vector[i] << " != " << i << std::endl;
            }
        }
    }
}
