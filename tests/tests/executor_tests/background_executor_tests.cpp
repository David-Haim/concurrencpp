#include "concurrencpp.h"
#include "../all_tests.h"

#include "../../tester/tester.h"
#include "../../helpers/assertions.h"
#include "../../helpers/object_observer.h"
#include "../../helpers/thread_helpers.h"

#include "executor_test_helpers.h"
#include "../../concurrencpp/src/executors/constants.h"

namespace concurrencpp::tests {
	void test_background_executor_name();
	void test_background_executor_enqueue();

	void test_background_executor_destructor_test_0();
	void test_background_executor_destructor_test_1();
	void test_background_executor_destructor();
}

void concurrencpp::tests::test_background_executor_name() {
	auto executor = concurrencpp::make_runtime()->background_executor();
	assert_same(executor->name(), concurrencpp::details::consts::k_background_executor_name);
}

void concurrencpp::tests::test_background_executor_enqueue() {
	object_observer observer;
	const size_t count = 10'000;

	concurrencpp::runtime_options opts;
	opts.max_background_threads = 10;
	opts.max_cpu_threads = 5;

	auto executor = concurrencpp::make_runtime(opts)->background_executor();

	for (size_t i = 0; i < count; i++) {
		executor->enqueue(observer.get_testing_stub());
	}

	assert_true(observer.wait_execution_count(count, std::chrono::seconds(60 * 2)));
	assert_true(observer.wait_destruction_count(count, std::chrono::seconds(60 * 2)));

	auto execution_map = observer.get_execution_map();

	assert_same(execution_map.size(), opts.max_background_threads);
}

void concurrencpp::tests::test_background_executor_destructor_test_0() {
	concurrencpp::runtime_options opts;
	opts.max_cpu_threads = 0;
	opts.max_background_threads = 1;

	auto executor = concurrencpp::make_runtime(opts)->background_executor();
	waiter waiter;
	object_observer observer;
	std::exception_ptr exception_ptr;

	executor->enqueue([waiter]() mutable {
		waiter.wait();
	});

	executor->enqueue(cancellation_tester(exception_ptr));
	
	waiter.notify_all();
	executor.reset();

	assert_cancelled_correctly<concurrencpp::errors::broken_task>(
		exception_ptr,
		concurrencpp::details::consts::k_background_executor_cancel_error_msg);
}

void concurrencpp::tests::test_background_executor_destructor_test_1() {
	waiter waiter;
	object_observer observer;
	concurrencpp::runtime_options opts;
	const auto thread_count = 4;
	const auto task_count = 128;

	opts.max_cpu_threads = 0;
	opts.max_background_threads = thread_count;

	auto executor = concurrencpp::make_runtime(opts)->background_executor();

	for (size_t i = 0; i < thread_count; i++) {
		executor->enqueue([waiter]() mutable {
			waiter.wait();
		});
	}

	for (size_t i = 0; i < task_count; i++) {
		executor->enqueue(observer.get_testing_stub());
	}

	waiter.notify_all();
	executor.reset(); 

	//threadpool destructor returns after all tasks are cancelled, no need to wait 
	assert_same(observer.get_execution_count(), 0);
	assert_same(observer.get_cancellation_count(), task_count);
	assert_same(observer.get_destruction_count(), task_count);
}

void concurrencpp::tests::test_background_executor_destructor() {
	test_background_executor_destructor_test_0();
	test_background_executor_destructor_test_1();
}

void concurrencpp::tests::test_background_executor() {
	tester tester("background_executor test");

	tester.add_step("name", test_background_executor_name);
	tester.add_step("enqueue", test_background_executor_enqueue);
	tester.add_step("~background_executor", test_background_executor_destructor);

	tester.launch_test();
}
