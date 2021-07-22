#include <iostream>
#include <chrono>

#include "concurrencpp/concurrencpp.h"

int main(int argc, const char* argv[]) {
    concurrencpp::runtime runtime;
    auto manual_executor = runtime.make_manual_executor();

    // This represents asynchronous, non-concurrencpp workers and agents that an application may use.
    // It is recommended to simply use other concurrencpp executors instead of creating raw threads and
    // user defined executors.
    std::vector<std::thread> threads;

    threads.emplace_back([manual_executor] {
        const auto tasks_executed = manual_executor->loop_for(10, std::chrono::seconds(30));
        std::cout << tasks_executed << " tasks were executed on thread " << std::this_thread::get_id() << std::endl;
    });

    threads.emplace_back([manual_executor] {
        const auto task_executed = manual_executor->loop_once_for(std::chrono::seconds(30));
        std::cout << (task_executed ? "one " : "no ") << "task was executed on thread " << std::this_thread::get_id() << std::endl;
    });

    threads.emplace_back([manual_executor] {
        std::cout << "thread id " << std::this_thread::get_id() << " is waiting for tasks to bo enqueued" << std::endl;
        auto num_of_tasks = manual_executor->wait_for_tasks_for(50, std::chrono::seconds(30));
        std::cout << "thread id " << std::this_thread::get_id() << ", there are " << num_of_tasks
                  << " enqueued tasks when returning from manual_executor::wait_for_tasks_for" << std::endl;

        num_of_tasks = manual_executor->loop(50);
        std::cout << "thread id " << std::this_thread::get_id() << ", executed " << num_of_tasks << " tasks " << std::endl;
    });

    struct dummy_task {
        void operator()() const noexcept {
            std::cout << "A dummy task is being executed on thread_id " << std::this_thread::get_id() << std::endl;
        }
    };

    threads.emplace_back([manual_executor] {
        std::vector<dummy_task> tasks(20);

        std::this_thread::sleep_for(std::chrono::seconds(5));

        std::cout << "thread_id " << std::this_thread::get_id() << " is posting " << tasks.size() << " tasks." << std::endl;
        manual_executor->bulk_post<dummy_task>(tasks);
    });

    threads.emplace_back([manual_executor] {
        std::vector<dummy_task> tasks(30);

        std::this_thread::sleep_for(std::chrono::seconds(12));

        std::cout << "thread_id " << std::this_thread::get_id() << " is posting " << tasks.size() << " tasks." << std::endl;
        manual_executor->bulk_post<dummy_task>(tasks);
    });

    threads.emplace_back([manual_executor] {
        std::vector<dummy_task> tasks(10);

        std::this_thread::sleep_for(std::chrono::seconds(18));

        std::cout << "thread_id " << std::this_thread::get_id() << " is posting " << tasks.size() << " tasks." << std::endl;
        manual_executor->bulk_post<dummy_task>(tasks);
    });

    std::this_thread::sleep_for(std::chrono::seconds(35));

    for (auto& thread : threads) {
        thread.join();
    }

    return 0;
}
