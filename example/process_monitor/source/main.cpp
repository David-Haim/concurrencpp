/*
        In this example, we will use concurrencpp::timer_queue and
   concurrencpp::timer to periodically monitor the state of the process, by
   logging the cpu and memory usage along with the thread count and the
   kernel-object count.

        Do note that this example uses a mock process monitor. the stats are
   made up and don't reflect the real state of this process. this example is
   good to show how concurrencpp:timer and concurrencpp::timer_queue can be used
        to schedule an asynchronous action periodically.
*/

#include "mock_process_monitor.h"
#include "concurrencpp/concurrencpp.h"

#include <iostream>

using namespace std::chrono_literals;

class process_stat_printer {

   private:
    mock_process_monitor::monitor m_process_monitor;

   public:
    void operator()() noexcept {
        const auto cpu_usage = m_process_monitor.cpu_usage();
        const auto memory_usage = m_process_monitor.memory_usage();
        const auto thread_count = m_process_monitor.thread_count();
        const auto kernel_object_count = m_process_monitor.kernel_object_count();

        std::cout << "cpu(%): " << cpu_usage << ", "
                  << "memory(%): " << memory_usage << ", "
                  << "thread count: " << thread_count << ", "
                  << "kernel-object count: " << kernel_object_count << std::endl;
    }
};

int main(int, const char**) {
    concurrencpp::runtime runtime;
    auto timer_queue = runtime.timer_queue();
    auto thread_pool_executor = runtime.thread_pool_executor();
    auto timer = timer_queue->make_timer(2500ms, 2500ms, thread_pool_executor, process_stat_printer());

    std::cout << "press enter to quit" << std::endl;
    std::getchar();
    return 0;
}
