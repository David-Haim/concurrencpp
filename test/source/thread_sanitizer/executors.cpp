#include "concurrencpp/concurrencpp.h"
#include "utils/executor_shutdowner.h"

#include <latch>
#include <chrono>
#include <iostream>

void test_executor_post(std::shared_ptr<concurrencpp::executor> executor, size_t tasks_per_thread = 100'000);
void test_executor_submit(std::shared_ptr<concurrencpp::executor> executor, size_t tasks_per_thread = 100'000);
void test_executor_bulk_post(std::shared_ptr<concurrencpp::executor> executor, size_t tasks_per_thread = 100'000);
void test_executor_bulk_submit(std::shared_ptr<concurrencpp::executor> executor, size_t tasks_per_thread = 100'000);

void test_manual_executor_wait_for_task(std::shared_ptr<concurrencpp::manual_executor> executor);
void test_manual_executor_wait_for_task_for(std::shared_ptr<concurrencpp::manual_executor> executor);
void test_manual_executor_wait_for_tasks(std::shared_ptr<concurrencpp::manual_executor> executor);
void test_manual_executor_wait_for_tasks_for(std::shared_ptr<concurrencpp::manual_executor> executor);
void test_manual_executor_loop_once(std::shared_ptr<concurrencpp::manual_executor> executor);
void test_manual_executor_loop_once_for(std::shared_ptr<concurrencpp::manual_executor> executor);
void test_manual_executor_loop(std::shared_ptr<concurrencpp::manual_executor> executor);
void test_manual_executor_loop_for(std::shared_ptr<concurrencpp::manual_executor> executor);
void test_manual_executor_clear(std::shared_ptr<concurrencpp::manual_executor> executor);

int main() {

    concurrencpp::runtime runtime;

    {
        std::cout << "\nStarting concurrencpp::inline_executor test.\n" << std::endl;

        auto ie = runtime.inline_executor();

        test_executor_post(ie);
        test_executor_submit(ie);
        test_executor_bulk_post(ie);
        test_executor_bulk_submit(ie);
    }

    {
        std::cout << "\nStarting concurrencpp::thread_pool_executor test.\n" << std::endl;

        auto tpe = runtime.thread_pool_executor();

        test_executor_post(tpe);
        test_executor_submit(tpe);
        test_executor_bulk_post(tpe);
        test_executor_bulk_submit(tpe);
    }

    {
        std::cout << "\nStarting concurrencpp::worker_thread_executor test.\n" << std::endl;

        test_executor_post(runtime.make_worker_thread_executor());
        test_executor_submit(runtime.make_worker_thread_executor());
        test_executor_bulk_post(runtime.make_worker_thread_executor());
        test_executor_bulk_submit(runtime.make_worker_thread_executor());
    }

    {
        std::cout << "\nStarting concurrencpp::thread_executor test.\n" << std::endl;

        auto te = runtime.thread_executor();

        test_executor_post(te, 216);
        test_executor_submit(te, 216);
        test_executor_bulk_post(te, 216);
        test_executor_bulk_submit(te, 216);
    }

    {
        std::cout << "\nStarting concurrencpp::manual_executor test.\n" << std::endl;

        test_executor_post(runtime.make_manual_executor());
        test_executor_submit(runtime.make_manual_executor());
        test_executor_bulk_post(runtime.make_manual_executor());
        test_executor_bulk_submit(runtime.make_manual_executor());
        test_manual_executor_wait_for_task(runtime.make_manual_executor());
        test_manual_executor_wait_for_task_for(runtime.make_manual_executor());
        test_manual_executor_wait_for_tasks(runtime.make_manual_executor());
        test_manual_executor_wait_for_tasks_for(runtime.make_manual_executor());
        test_manual_executor_loop_once(runtime.make_manual_executor());
        test_manual_executor_loop_once_for(runtime.make_manual_executor());
        test_manual_executor_loop(runtime.make_manual_executor());
        test_manual_executor_loop_for(runtime.make_manual_executor());
        test_manual_executor_clear(runtime.make_manual_executor());
    }
}

using namespace concurrencpp;
using namespace std::chrono;

/*
 * When we test manual_executor, we need to inject to the test a group of threads that act
 * as the consumers, otherwise no-one will execute the test tasks.
 */
std::vector<std::thread> maybe_inject_consumer_threads(std::shared_ptr<concurrencpp::executor> executor,
                                                       time_point<system_clock> production_tp,
                                                       const size_t num_of_threads,
                                                       const size_t tasks_per_thread) {
    std::vector<std::thread> maybe_consumer_threads;
    const auto maybe_manual_executor = std::dynamic_pointer_cast<concurrencpp::manual_executor>(executor);

    if (!static_cast<bool>(maybe_manual_executor)) {
        return maybe_consumer_threads;
    }

    maybe_consumer_threads.reserve(num_of_threads);

    for (size_t i = 0; i < num_of_threads; i++) {
        maybe_consumer_threads.emplace_back([=] {
            std::this_thread::sleep_until(production_tp - milliseconds(1));
            maybe_manual_executor->loop_for(tasks_per_thread, minutes(10));
        });
    }

    return maybe_consumer_threads;
}

void test_executor_post(std::shared_ptr<concurrencpp::executor> executor, size_t tasks_per_thread) {
    std::cout << executor->name << "::post" << std::endl;

    const auto post_tp = system_clock::now() + milliseconds(200);
    const auto num_of_threads = std::thread::hardware_concurrency() * 4;
    const auto total_task_count = tasks_per_thread * num_of_threads;

    std::latch latch(total_task_count);

    std::vector<std::thread> poster_threads;
    poster_threads.resize(num_of_threads);

    for (auto& thread : poster_threads) {
        thread = std::thread([=, &latch] {
            std::this_thread::sleep_until(post_tp);

            for (size_t i = 0; i < tasks_per_thread; i++) {
                executor->post([&latch]() mutable {
                    latch.count_down();
                });
            }
        });
    }

    auto maybe_consumer_threads = maybe_inject_consumer_threads(executor, post_tp, num_of_threads, tasks_per_thread);

    latch.wait();

    for (auto& thread : poster_threads) {
        thread.join();
    }

    for (auto& thread : maybe_consumer_threads) {
        thread.join();
    }

    std::cout << "===================================" << std::endl;
}

void test_executor_submit(std::shared_ptr<concurrencpp::executor> executor, size_t tasks_per_thread) {
    std::cout << executor->name << "::submit" << std::endl;

    const auto submit_tp = system_clock::now() + milliseconds(200);
    const auto num_of_threads = std::thread::hardware_concurrency() * 4;

    std::vector<std::thread> submitter_threads;
    submitter_threads.resize(num_of_threads);

    for (auto& thread : submitter_threads) {
        thread = std::thread([=]() mutable {
            std::vector<result<size_t>> results;
            results.reserve(tasks_per_thread);

            std::this_thread::sleep_until(submit_tp);

            for (size_t i = 0; i < tasks_per_thread; i++) {
                results.emplace_back(executor->submit([i] {
                    return i;
                }));
            }

            for (size_t i = 0; i < tasks_per_thread; i++) {
                const auto val = results[i].get();
                if (val != i) {
                    std::cerr << "submit test failed, submitted " << i << " got " << val << std::endl;
                    std::abort();
                }
            }
        });
    }

    auto maybe_consumer_threads = maybe_inject_consumer_threads(executor, submit_tp, num_of_threads, tasks_per_thread);

    for (auto& thread : submitter_threads) {
        thread.join();
    }

    for (auto& thread : maybe_consumer_threads) {
        thread.join();
    }

    std::cout << "===================================" << std::endl;
}

void test_executor_bulk_post(std::shared_ptr<concurrencpp::executor> executor, size_t tasks_per_thread) {
    std::cout << executor->name << "::bulk_post" << std::endl;

    const auto post_tp = system_clock::now() + milliseconds(250);
    const auto num_of_threads = std::thread::hardware_concurrency() * 4;
    const auto total_task_count = tasks_per_thread * num_of_threads;

    std::latch latch(total_task_count);

    std::vector<std::thread> poster_threads;
    poster_threads.resize(num_of_threads);

    for (auto& thread : poster_threads) {
        thread = std::thread([=, &latch] {
            auto task = [&latch]() mutable {
                latch.count_down();
            };

            std::vector<decltype(task)> tasks;
            tasks.reserve(tasks_per_thread);

            for (size_t i = 0; i < tasks_per_thread; i++) {
                tasks.emplace_back(task);
            }

            std::this_thread::sleep_until(post_tp);

            executor->bulk_post<decltype(task)>(tasks);
        });
    }

    auto maybe_consumer_threads = maybe_inject_consumer_threads(executor, post_tp, num_of_threads, tasks_per_thread);

    latch.wait();

    for (auto& thread : poster_threads) {
        thread.join();
    }

    for (auto& thread : maybe_consumer_threads) {
        thread.join();
    }

    std::cout << "===================================" << std::endl;
}

struct val_returner {
    const size_t val;

    val_returner(size_t val) noexcept : val(val) {}
    val_returner(const val_returner&) noexcept = default;

    size_t operator()() const noexcept {
        return val;
    }
};

void test_executor_bulk_submit(std::shared_ptr<concurrencpp::executor> executor, size_t tasks_per_thread) {
    std::cout << executor->name << "::bulk_submit" << std::endl;

    const auto submit_tp = system_clock::now() + milliseconds(250);
    const auto num_of_threads = std::thread::hardware_concurrency() * 4;

    std::vector<std::thread> submitter_threads;
    submitter_threads.resize(num_of_threads);

    for (auto& thread : submitter_threads) {
        thread = std::thread([=]() mutable {
            std::vector<val_returner> tasks;
            tasks.reserve(tasks_per_thread);

            for (size_t i = 0; i < tasks_per_thread; i++) {
                tasks.emplace_back(i);
            }

            std::this_thread::sleep_until(submit_tp);

            auto results = executor->bulk_submit<val_returner>(tasks);

            for (size_t i = 0; i < tasks_per_thread; i++) {
                const auto val = results[i].get();
                if (val != i) {
                    std::cerr << "bulk_submit test failed, submitted " << i << " got " << val << std::endl;
                    std::abort();
                }
            }
        });
    }

    auto maybe_consumer_threads = maybe_inject_consumer_threads(executor, submit_tp, num_of_threads, tasks_per_thread);

    for (auto& thread : submitter_threads) {
        thread.join();
    }

    for (auto& thread : maybe_consumer_threads) {
        thread.join();
    }

    std::cout << "===================================" << std::endl;
}

void test_manual_executor_wait_for_task(std::shared_ptr<concurrencpp::manual_executor> executor) {
    std::cout << "manual_executor::wait_for_task" << std::endl;

    const auto num_of_threads = std::thread::hardware_concurrency() * 10;

    std::vector<std::thread> waiter_threads;
    waiter_threads.resize(num_of_threads);

    for (auto& thread : waiter_threads) {
        thread = std::thread([executor]() mutable {
            executor->wait_for_task();
        });
    }

    std::this_thread::sleep_for(milliseconds(200));

    executor->post([] {
    });

    for (auto& thread : waiter_threads) {
        thread.join();
    }

    std::cout << "===================================" << std::endl;
}

void test_manual_executor_wait_for_task_for(std::shared_ptr<concurrencpp::manual_executor> executor) {
    std::cout << "manual_executor::wait_for_task_for" << std::endl;

    const auto num_of_threads = std::thread::hardware_concurrency() * 10;

    std::vector<std::thread> waiter_threads;
    waiter_threads.resize(num_of_threads);
    size_t time_to_sleep_counter = 1;

    for (auto& thread : waiter_threads) {
        thread = std::thread([executor, time_to_sleep_counter]() mutable {
            const auto time = milliseconds(time_to_sleep_counter * 10);
            executor->wait_for_task_for(time);
        });

        ++time_to_sleep_counter;
    }

    std::this_thread::sleep_for(milliseconds(time_to_sleep_counter * 5));

    executor->post([] {
    });

    for (auto& thread : waiter_threads) {
        thread.join();
    }

    std::cout << "===================================" << std::endl;
}

void test_manual_executor_wait_for_tasks(std::shared_ptr<concurrencpp::manual_executor> executor) {
    std::cout << "manual_executor::wait_for_tasks" << std::endl;

    const auto num_of_threads = std::thread::hardware_concurrency() * 10;

    std::vector<std::thread> waiter_threads;
    waiter_threads.resize(num_of_threads);
    size_t task_count_counter = 1;

    for (auto& thread : waiter_threads) {
        thread = std::thread([executor, task_count_counter]() mutable {
            executor->wait_for_tasks(task_count_counter * 10);
        });

        ++task_count_counter;
    }

    std::this_thread::sleep_for(milliseconds(250));

    for (size_t i = 0; i < task_count_counter * 10; i++) {
        executor->post([] {
        });
    }

    for (auto& thread : waiter_threads) {
        thread.join();
    }

    std::cout << "===================================" << std::endl;
}

void test_manual_executor_wait_for_tasks_for(std::shared_ptr<concurrencpp::manual_executor> executor) {
    std::cout << "manual_executor::wait_for_tasks_for" << std::endl;

    const auto num_of_threads = std::thread::hardware_concurrency() * 10;

    std::vector<std::thread> waiter_threads;
    waiter_threads.resize(num_of_threads);
    size_t counter = 1;

    for (auto& thread : waiter_threads) {
        thread = std::thread([executor, counter]() mutable {
            const auto time = milliseconds(counter * 10);
            executor->wait_for_tasks_for(counter * 10, time);
        });

        ++counter;
    }

    std::this_thread::sleep_for(milliseconds(counter * 5));

    for (size_t i = 0; i < counter * 10; i++) {
        executor->post([] {
        });
    }

    for (auto& thread : waiter_threads) {
        thread.join();
    }

    std::cout << "===================================" << std::endl;
}

void test_manual_executor_loop_once(std::shared_ptr<concurrencpp::manual_executor> executor) {
    std::cout << "manual_executor::loop_once" << std::endl;

    const auto num_of_threads = std::thread::hardware_concurrency() * 10;
    const auto looping_tp = system_clock::now() + milliseconds(250);

    std::vector<std::thread> looping_threads;
    looping_threads.resize(num_of_threads);

    for (auto& thread : looping_threads) {
        thread = std::thread([executor, looping_tp]() mutable {
            std::this_thread::sleep_until(looping_tp);
            executor->loop_once();
        });
    }

    std::this_thread::sleep_until(looping_tp);

    for (size_t i = 0; i < num_of_threads; i++) {
        executor->post([] {
        });
    }

    for (auto& thread : looping_threads) {
        thread.join();
    }

    std::cout << "===================================" << std::endl;
}

void test_manual_executor_loop_once_for(std::shared_ptr<concurrencpp::manual_executor> executor) {
    std::cout << "manual_executor::loop_once_for" << std::endl;

    const auto num_of_threads = std::thread::hardware_concurrency() * 10;

    std::vector<std::thread> waiter_threads;
    waiter_threads.resize(num_of_threads);
    size_t time_to_sleep_counter = 1;

    for (auto& thread : waiter_threads) {
        thread = std::thread([executor, time_to_sleep_counter]() mutable {
            const auto time = milliseconds(time_to_sleep_counter * 10);
            executor->loop_once_for(time);
        });

        ++time_to_sleep_counter;
    }

    std::this_thread::sleep_for(milliseconds(time_to_sleep_counter * 5));

    for (size_t i = 0; i < num_of_threads; i++) {
        executor->post([] {
        });
    }

    for (auto& thread : waiter_threads) {
        thread.join();
    }

    std::cout << "===================================" << std::endl;
}

void test_manual_executor_loop(std::shared_ptr<concurrencpp::manual_executor> executor) {
    std::cout << "manual_executor::loop" << std::endl;

    const auto num_of_threads = std::thread::hardware_concurrency() * 10;
    const auto looping_tp = system_clock::now() + milliseconds(250);

    std::vector<std::thread> waiter_threads;
    waiter_threads.resize(num_of_threads);
    size_t task_count_counter = 1;

    for (auto& thread : waiter_threads) {
        thread = std::thread([executor, task_count_counter, looping_tp]() mutable {
            std::this_thread::sleep_until(looping_tp);
            executor->loop(task_count_counter * 10);
        });

        ++task_count_counter;
    }

    std::this_thread::sleep_until(looping_tp);

    for (size_t i = 0; i < task_count_counter * 100; i++) {
        executor->post([] {
        });
    }

    for (auto& thread : waiter_threads) {
        thread.join();
    }

    std::cout << "===================================" << std::endl;
}

void test_manual_executor_loop_for(std::shared_ptr<concurrencpp::manual_executor> executor) {
    std::cout << "manual_executor::loop_for" << std::endl;

    const auto num_of_threads = std::thread::hardware_concurrency() * 10;

    std::vector<std::thread> waiter_threads;
    waiter_threads.resize(num_of_threads);
    size_t counter = 1;

    for (auto& thread : waiter_threads) {
        thread = std::thread([executor, counter]() mutable {
            const auto time = milliseconds(counter * 10);
            executor->loop_for(counter * 10, time);
        });

        ++counter;
    }

    std::this_thread::sleep_for(milliseconds(counter * 5));

    for (size_t i = 0; i < counter * 100; i++) {
        executor->post([] {
        });
    }

    for (auto& thread : waiter_threads) {
        thread.join();
    }

    std::cout << "===================================" << std::endl;
}

void test_manual_executor_clear(std::shared_ptr<concurrencpp::manual_executor> executor) {
    std::cout << "manual_executor::loop_for" << std::endl;

    const auto num_of_threads = std::thread::hardware_concurrency() * 10;

    std::vector<std::thread> threads;
    threads.reserve(num_of_threads);

    const auto clear_tp = high_resolution_clock::now() + milliseconds(250);

    for (size_t i = 0; i < num_of_threads / 2; i++) {
        threads.emplace_back([executor, clear_tp]() mutable {
            std::this_thread::sleep_until(clear_tp);

            for (size_t j = 0; j < 100; j++) {
                executor->clear();
            }
        });
    }

    for (size_t i = 0; i < num_of_threads / 2; i++) {
        threads.emplace_back([executor, clear_tp]() mutable {
            std::this_thread::sleep_until(clear_tp - milliseconds(1));

            for (size_t i = 0; i < 10'000; i++) {
                executor->post([] {
                });

                std::this_thread::yield();
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    std::cout << "===================================" << std::endl;
}
