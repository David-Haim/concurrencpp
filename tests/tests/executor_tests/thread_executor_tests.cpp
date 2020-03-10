#include "concurrencpp.h"
#include "../all_tests.h"

#include "../../tester/tester.h"
#include "../../helpers/assertions.h"
#include "../../helpers/object_observer.h"

#include "../../concurrencpp/details/executors/constants.h"

namespace concurrencpp::tests {
	void test_thread_executor_name();
	void test_thread_executor_enqueue();
	void test_thread_executor_destructor();
}

void concurrencpp::tests::test_thread_executor_name() {
	auto executor = concurrencpp::make_runtime()->thread_executor();
	assert_same(executor->name(), concurrencpp::details::consts::k_thread_executor_name);
}

void concurrencpp::tests::test_thread_executor_enqueue() {
	object_observer observer;
	const size_t count = 128;
	auto executor = concurrencpp::make_runtime()->thread_executor();

	for (size_t i = 0; i < count; i++) {
		executor->enqueue(observer.get_testing_stub(std::chrono::milliseconds(75)));
	}

	assert_true(observer.wait_execution_count(count, std::chrono::minutes(4)));
	assert_true(observer.wait_destruction_count(count, std::chrono::minutes(4)));

	auto execution_map = observer.get_execution_map();

	assert_same(execution_map.size(), count);
}

void concurrencpp::tests::test_thread_executor_destructor() {
	//the executor returns only when all tasks are done.
	auto executor = concurrencpp::make_runtime()->thread_executor();
	object_observer observer;
	const size_t count = 16;

	for (size_t i = 0; i < count; i++) {
		executor->enqueue(observer.get_testing_stub());
	}

	executor.reset();

	assert_same(observer.get_execution_count(), count);
	assert_same(observer.get_destruction_count(), count);
}

void concurrencpp::tests::test_thread_executor() {
	tester tester("thread_executor test");

	tester.add_step("name", test_thread_executor_name);
	tester.add_step("enqueue", test_thread_executor_enqueue);
	tester.add_step("~thread_executor", test_thread_executor_destructor);

	tester.launch_test();
}
