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

concurrencpp::result<void> consume_tasks_as_they_finish(std::shared_ptr<concurrencpp::thread_pool_executor> resume_executor,
                                                        std::vector<concurrencpp::result<int>> results) {
    while (!results.empty()) {
        auto when_any = co_await concurrencpp::when_any(resume_executor, results.begin(), results.end());
        auto finished_task = std::move(when_any.results[when_any.index]);

        const auto done_value = co_await finished_task;

        auto msg =
            std::string("task num: ") + std::to_string(when_any.index) + " is finished with value of: " + std::to_string(done_value);

        std::cout << msg << std::endl;

        results = std::move(when_any.results);
        results.erase(results.begin() + when_any.index);
    }
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

    consume_tasks_as_they_finish(thread_pool_executor, std::move(results)).get();
    return 0;
}
