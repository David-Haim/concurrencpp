#include "mock_web_socket.h"

#include <chrono>
#include <thread>
#include <stdexcept>

#include <cstdlib>
#include <ctime>

namespace mock_web_socket {
    bool failed() noexcept {
        static const int dummy = [] {
            std::srand(static_cast<unsigned int>(std::time(nullptr)));
            return 0;
        }();
        (void)dummy;

        const auto randomized_num = std::rand() % 100;
        return randomized_num <= 5;
    }

    size_t random_in_range(size_t min, size_t max) {
        const auto range = max - min + 1;
        return std::rand() % range + min;
    }

    const std::string cities[] =
        {"London", "New York City", "Tokyo", "Paris", "Singapore", "Amsterdam", "Seoul", "Berlin", "Hong Kong", "Sydney"};
}  // namespace mock_web_socket

void mock_web_socket::web_socket::open(std::string_view) {
    if (failed()) {
        throw std::runtime_error("web_socket::open - can't connect.");
    }

    std::this_thread::sleep_for(std::chrono::seconds(2));
}

std::string mock_web_socket::web_socket::receive_msg() {
    const auto time_to_sleep = random_in_range(0, 4);
    std::this_thread::sleep_for(std::chrono::seconds(time_to_sleep));

    const auto random_city = cities[random_in_range(0, std::size(cities) - 1)];
    const auto random_temperature = random_in_range(-5, 37);

    return std::string("Current temperature in ") + random_city + " is " + std::to_string(random_temperature) + " deg. C";
}
