#include "infra/tester.h"

#include <chrono>

#include <cstdio>

using concurrencpp::tests::test_step;
using concurrencpp::tests::tester;
using namespace std::chrono;

test_step::test_step(const char* step_name, std::function<void()> callable) : m_step_name(step_name), m_step(std::move(callable)) {}

void test_step::launch_test_step() noexcept {
    const auto test_start_time = system_clock::now();
    printf("\tTest-step started: %s\n", m_step_name);

    m_step();

    const auto elapsed_time = duration_cast<milliseconds>(system_clock::now() - test_start_time).count();
    printf("\tTest-step ended (%lldms).\n", elapsed_time);
}

tester::tester(const char* test_name) noexcept : m_test_name(test_name) {}

void tester::add_step(const char* step_name, std::function<void()> callable) {
    m_steps.emplace_back(step_name, std::move(callable));
}

void tester::launch_test() noexcept {
    printf("Test started: %s\n", m_test_name);

    for (auto& test_step : m_steps) {
        try {
            test_step.launch_test_step();
        } catch (const std::exception& ex) {
            ::fprintf(stderr, "\tTest step terminated with an exception : %s\n", ex.what());
        }
    }

    printf("Test ended.\n____________________\n");
}
