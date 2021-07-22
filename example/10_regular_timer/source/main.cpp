#include "concurrencpp/concurrencpp.h"

#include <iostream>

using namespace std::chrono_literals;

int main() {
    concurrencpp::runtime runtime;
    std::atomic_size_t counter = 1;
    concurrencpp::timer timer = runtime.timer_queue()->make_timer(1500ms, 2000ms, runtime.thread_pool_executor(), [&] {
        const auto c = counter.fetch_add(1);
        std::cout << "timer was invoked for the " << c << "th time" << std::endl;
    });

    std::cout << "timer due time (ms): " << timer.get_due_time().count() << std::endl;
    std::cout << "timer frequency (ms): " << timer.get_frequency().count() << std::endl;
    std::cout << "timer-associated executor : " << timer.get_executor()->name << std::endl;

    std::this_thread::sleep_for(20s);

    std::cout << "main thread cancelling timer" << std::endl;
    timer.cancel();

    std::this_thread::sleep_for(10s);

    return 0;
}