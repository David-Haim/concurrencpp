#include "infra/tester.h"

#include <chrono>
#include <iostream>

using concurrencpp::tests::test_step;
using concurrencpp::tests::tester;
using namespace std::chrono;

test_step::test_step(const char* step_name, std::function<void()> callable) : m_step_name(step_name), m_step(std::move(callable)) {}

void test_step::launch_test_step() noexcept {
    const auto test_start_time = system_clock::now();
    std::cout << "\tTest-step started: " << m_step_name << std::endl;

    m_step();

    const auto elapsed_time = duration_cast<milliseconds>(system_clock::now() - test_start_time).count();
    std::cout << "\tTest-step ended (" << elapsed_time << "ms)." << std::endl;
}

tester::tester(const char* test_name) noexcept : m_test_name(test_name) {}

void tester::add_step(const char* step_name, std::function<void()> callable) {
    m_steps.emplace_back(step_name, std::move(callable));
}

void tester::launch_test() noexcept {
    std::cout << "Test started: " << m_test_name << std::endl;

    for (auto& test_step : m_steps) {
        try {
            test_step.launch_test_step();
        } catch (const std::exception& ex) {
            std::cerr << "\tTest step terminated with an exception : " << ex.what() << std::endl;
        }
    }

    std::cout << "Test ended.\n____________________" << std::endl;
}
