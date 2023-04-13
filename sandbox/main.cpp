#include "concurrencpp/concurrencpp.h"

#include <iostream>

using namespace concurrencpp;
int fibonacci_sync(int i) {
    if (i == 0) {
        return 0;
    }

    if (i == 1) {
        return 1;
    }

    return fibonacci_sync(i - 1) + fibonacci_sync(i - 2);
}

result<int> fibonacci(executor_tag, std::shared_ptr<thread_pool_executor> tpe, const int curr) {
    if (curr <= 10) {
        co_return fibonacci_sync(curr);
    }

    auto fib_1 = fibonacci({}, tpe, curr - 1);
    auto fib_2 = fibonacci({}, tpe, curr - 2);

    co_return co_await fib_1 + co_await fib_2;
}

int main() {
    concurrencpp::runtime runtime;
    auto fibb_30 = fibonacci({}, runtime.thread_pool_executor(), 30).get();
    std::cout << "fibonacci(30) = " << fibb_30 << std::endl;
    return 0;
}