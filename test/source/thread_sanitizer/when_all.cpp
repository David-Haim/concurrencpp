#include "concurrencpp/concurrencpp.h"

#include <iostream>

concurrencpp::result<void> when_all_test(
    std::shared_ptr<concurrencpp::thread_executor> tpe);

int main() {
    concurrencpp::runtime runtime;
    when_all_test(runtime.thread_executor()).get();
    return 0;
}

using namespace concurrencpp;

std::vector<result<int>> run_loop_once(std::shared_ptr<thread_executor> tpe) {
    std::vector<result<int>> results;
    results.reserve(1'024);

    for (size_t i = 0; i < 1'024; i++) {
        results.emplace_back(tpe->submit([] {
            std::this_thread::yield();
            return 0;
        }));
    }

    return results;
}

void assert_loop(result<std::vector<result<int>>> result_list) {
    if (result_list.status() != result_status::value) {
        std::abort();
    }

    auto results = result_list.get();

    for (auto& result : results) {
        if (result.get() != 0) {
            std::abort();
        }
    }
}

result<void> when_all_test(std::shared_ptr<concurrencpp::thread_executor> tpe) {
    auto loop_0 = run_loop_once(tpe);
    auto loop_1 = run_loop_once(tpe);
    auto loop_2 = run_loop_once(tpe);
    auto loop_3 = run_loop_once(tpe);
    auto loop_4 = run_loop_once(tpe);

    auto loop_0_all = when_all(loop_0.begin(), loop_0.end());
    auto loop_1_all = when_all(loop_1.begin(), loop_1.end());
    auto loop_2_all = when_all(loop_2.begin(), loop_2.end());
    auto loop_3_all = when_all(loop_3.begin(), loop_3.end());
    auto loop_4_all = when_all(loop_4.begin(), loop_4.end());

    auto all = when_all(std::move(loop_0_all), std::move(loop_1_all), std::move(loop_2_all), std::move(loop_3_all), std::move(loop_4_all));

    auto all_done = co_await all;

    assert_loop(std::move(std::get<0>(all_done)));
    assert_loop(std::move(std::get<1>(all_done)));
    assert_loop(std::move(std::get<2>(all_done)));
    assert_loop(std::move(std::get<3>(all_done)));
    assert_loop(std::move(std::get<4>(all_done)));
}
