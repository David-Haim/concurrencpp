#include "concurrencpp.h"
#include "../all_tests.h"

#include "../../tester/tester.h"
#include "../../helpers/assertions.h"
#include "../../helpers/object_observer.h"

#include "../../concurrencpp/src/executors/constants.h"

namespace concurrencpp::tests {
	void test_inline_executor_name();
	void test_inline_executor_enqueue();
}

void concurrencpp::tests::test_inline_executor_name() {
	auto executor = concurrencpp::make_runtime()->inline_executor();
	assert_same(executor->name(), concurrencpp::details::consts::k_inline_executor_name);
}

void concurrencpp::tests::test_inline_executor_enqueue() {
	object_observer observer;
	const size_t count = 1'000;
	auto executor = concurrencpp::make_runtime()->inline_executor();
	
	for (size_t i = 0; i < count; i++) {
		executor->enqueue(observer.get_testing_stub());
	}

	assert_same(observer.get_execution_count(), count);
	assert_same(observer.get_destruction_count(), count);

	auto execution_map = observer.get_execution_map();

	assert_same(execution_map.size(), 1);
	assert_same(execution_map.begin()->first, std::this_thread::get_id());
}

void concurrencpp::tests::test_inline_executor() {
	tester tester("inline_executor test");

	tester.add_step("name", test_inline_executor_name);
	tester.add_step("enqueue", test_inline_executor_enqueue);

	tester.launch_test();
}
