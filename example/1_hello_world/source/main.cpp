#include "concurrencpp/concurrencpp.h"
#include <iostream>

int main() {
    concurrencpp::runtime runtime;
    auto result = runtime.thread_executor()->submit([] {
        std::cout << "hello world" << std::endl;
    });

    result.get();
    return 0;
}