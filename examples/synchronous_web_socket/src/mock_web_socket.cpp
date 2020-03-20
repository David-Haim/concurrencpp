#include "mock_web_socket.h"

#include <chrono>
#include <thread>
#include <stdexcept>

#include <cstdlib>
#include <ctime>

namespace mock_web_socket {

	static const int dummy = [] {
		std::srand(::time(nullptr));
		return 0;
	}();

	bool failed() noexcept {		
		const auto randomized_num = std::rand() % 100;
		return randomized_num <= 5;
	}

	int random_in_range(int min, int max) {
		int range = max - min + 1;
		return std::rand() % range + min;
	}

	const std::string cities[] = {
		"London",
		"New York City",
		"Tokyo",
		"Paris",
		"Singapore",
		"Amsterdam",
		"Seoul",
		"Berlin",
		"Hong Kong",
		"Sydney"
	};
}

void mock_web_socket::web_socket::open(std::string_view url){
	if (failed()) {
		throw std::runtime_error("web_socket::open - can't connect.");
	}

	std::this_thread::sleep_for(std::chrono::seconds(2));
}

std::string mock_web_socket::web_socket::receive_msg(){
	const auto time_to_sleep = random_in_range(2, 8);
	std::this_thread::sleep_for(std::chrono::seconds(time_to_sleep));

	auto random_city = cities[random_in_range(0, std::size(cities) - 1)];
	auto random_temprature = random_in_range(-5, 37);

	return std::string("Current temprature in ") + random_city + " is " + std::to_string(random_temprature) + " deg. C";
}