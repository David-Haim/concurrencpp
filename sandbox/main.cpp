#include "concurrencpp.h"

#include <iostream>

int main() {
	auto runtime = concurrencpp::make_runtime();
	auto result = runtime->thread_pool_executor()->submit([] {
		std::cout << "hello world" << std::endl;
	});

	result.get();
	return 0;
}
