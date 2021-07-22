#include <iostream>

#include "concurrencpp/concurrencpp.h"

using namespace std::chrono_literals;

concurrencpp::null_result delayed_task(std::shared_ptr<concurrencpp::timer_queue> tq,
                                       std::shared_ptr<concurrencpp::thread_pool_executor> ex) {
    size_t counter = 1;

    while (true) {
        std::cout << "task was invoked " << counter << " times." << std::endl;
        counter++;

        co_await tq->make_delay_object(1500ms, ex);
    }
}

int main() {
    concurrencpp::runtime runtime;
    delayed_task(runtime.timer_queue(), runtime.thread_pool_executor());

    std::this_thread::sleep_for(10s);
    return 0;
}