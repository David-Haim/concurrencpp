#include "concurrencpp.h"
#include "all_tests.h"

#include "../tester/tester.h"
#include "../helpers/assertions.h"

namespace concurrencpp::tests {
	void test_runtime_destructor();
	void test_runtime_version();
}

namespace concurrencpp::tests {
	struct dummy_executor : public concurrencpp::executor {

		bool shutdown_requested_flag = false;

		dummy_executor(const char* name, int, float) : executor(name) {}

		void enqueue(std::experimental::coroutine_handle<>) override {}
		void enqueue(std::span<std::experimental::coroutine_handle<>>) override {}

		int max_concurrency_level() const noexcept override { return 0; }

		bool shutdown_requested() const noexcept override { return shutdown_requested_flag; };
		void shutdown() noexcept override { shutdown_requested_flag = true; };
	};
}

void concurrencpp::tests::test_runtime_destructor() {
	std::shared_ptr<concurrencpp::executor> executors[7];

	{
		concurrencpp::runtime runtime;
		executors[0] = runtime.inline_executor();
		executors[1] = runtime.thread_pool_executor();
		executors[2] = runtime.background_executor();
		executors[3] = runtime.thread_executor();
		executors[4] = runtime.make_worker_thread_executor();
		executors[5] = runtime.make_manual_executor();
		executors[6] = runtime.template make_executor<dummy_executor>("dummy_executor", 1, 4.4f);

		for (auto& executor : executors) {
			assert_true(static_cast<bool>(executor));
			assert_false(executor->shutdown_requested());
		}
	}

	for (auto& executor : executors) {
		assert_true(executor->shutdown_requested());
	}
}

void concurrencpp::tests::test_runtime_version() {
	concurrencpp::runtime runtime;

	auto version = runtime.version();
	assert_equal(std::get<0>(version), concurrencpp::details::consts::k_concurrencpp_version_major);
	assert_equal(std::get<1>(version), concurrencpp::details::consts::k_concurrencpp_version_minor);
	assert_equal(std::get<2>(version), concurrencpp::details::consts::k_concurrencpp_version_revision);
}

void concurrencpp::tests::test_runtime() {
	tester tester("runtime test");

	tester.add_step("~runtime", test_runtime_destructor);
	tester.add_step("version", test_runtime_version);

	tester.launch_test();
}