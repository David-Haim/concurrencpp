#include "concurrencpp/concurrencpp.h"

#include <iostream>

#include <ctime>
#include <cstdlib>

int main() {
    concurrencpp::result_promise<std::string> promise;
    auto result = promise.get_result();

    std::thread my_3_party_executor([promise = std::move(promise)]() mutable {
        std::this_thread::sleep_for(std::chrono::seconds(1));  // imitate real work

        // imitate a random failure
        std::srand(static_cast<unsigned>(::time(nullptr)));
        if (std::rand() % 100 < 90) {
            promise.set_result("hello world");
        } else {
            promise.set_exception(std::make_exception_ptr(std::runtime_error("failure")));
        }
    });

    try {
        auto asynchronous_string = result.get();
        std::cout << "result promise returned string: " << asynchronous_string << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "An exception was thrown while executing asynchronous function: " << e.what() << std::endl;
    }

    my_3_party_executor.join();
}