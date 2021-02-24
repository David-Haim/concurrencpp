#include "concurrencpp/concurrencpp.h"

#include <iostream>

concurrencpp::result<int> fibbonacci(concurrencpp::executor_tag, std::shared_ptr<concurrencpp::thread_pool_executor> tpe, int curr);
int fibbonacci_sync(int i) noexcept;

int main() {
    std::cout << "Starting parallel Fibonacci test" << std::endl;

    const auto fibb_sync = fibbonacci_sync(32);

    concurrencpp::runtime_options opts;
    opts.max_cpu_threads = std::thread::hardware_concurrency() * 8;
    concurrencpp::runtime runtime(opts);

    const auto fibb = fibbonacci({}, runtime.thread_pool_executor(), 32).get();
    if (fibb != fibb_sync) {
        std::cerr << "fibonacci test failed. expected " << fibb_sync << " got " << fibb << std::endl;
        std::abort();
    }

    std::cout << "fibonacci(32) = " << fibb << std::endl;
    std::cout << "================================" << std::endl;
}

using namespace concurrencpp;

result<int> fibbonacci(executor_tag, std::shared_ptr<thread_pool_executor> tpe, const int curr) {
    if (curr == 1) {
        co_return 1;
    }

    if (curr == 0) {
        co_return 0;
    }

    auto fib_1 = fibbonacci({}, tpe, curr - 1);
    auto fib_2 = fibbonacci({}, tpe, curr - 2);

    co_return co_await fib_1 + co_await fib_2;
}

int fibbonacci_sync(int i) noexcept {
    if (i == 0) {
        return 0;
    }

    if (i == 1) {
        return 1;
    }

    return fibbonacci_sync(i - 1) + fibbonacci_sync(i - 2);
}
