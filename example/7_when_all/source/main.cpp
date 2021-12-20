#include <iostream>
#include <chrono>
#include <string>

#include <ctime>

#include "concurrencpp/concurrencpp.h"

int example_job(int task_num, int dummy_value, int sleeping_time_ms) {
    const auto msg0 =
        std::string("task num: ") + std::to_string(task_num) + " is going to sleep for " + std::to_string(sleeping_time_ms) + " ms";
    std::cout << msg0 << std::endl;

    std::this_thread::sleep_for(std::chrono::milliseconds(sleeping_time_ms));

    const auto msg1 = std::string("task num: ") + std::to_string(task_num) + " woke up.";
    std::cout << msg1 << std::endl;

    return dummy_value;
}

concurrencpp::result<void> consume_all_tasks(std::shared_ptr<concurrencpp::thread_pool_executor> resume_executor,
                                             std::vector<concurrencpp::result<int>> results) {
    auto all_done = co_await concurrencpp::when_all(resume_executor, results.begin(), results.end());

    for (auto& done_result : all_done) {
        std::cout << co_await done_result << std::endl;
    }

    std::cout << "all tasks were consumed" << std::endl;
}

int main(int argc, const char* argv[]) {
    concurrencpp::runtime runtime;
    auto background_executor = runtime.background_executor();
    auto thread_pool_executor = runtime.thread_pool_executor();
    std::vector<concurrencpp::result<int>> results;

    std::srand(static_cast<unsigned>(std::time(nullptr)));

    for (int i = 0; i < 10; i++) {
        const auto sleeping_time_ms = std::rand() % 1000;
        results.emplace_back(background_executor->submit(example_job, i, i * 15, sleeping_time_ms));
    }

    consume_all_tasks(thread_pool_executor, std::move(results)).get();
    return 0;
}
