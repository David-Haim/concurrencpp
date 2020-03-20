#include "concurrencpp.h"
#include "../all_tests.h"

#include "../../tester/tester.h"
#include "../../helpers/assertions.h"
#include "../../helpers/object_observer.h"
#include "../../helpers/thread_helpers.h"

#include "executor_test_helpers.h"
#include "../../concurrencpp/src/executors/constants.h"

namespace concurrencpp::tests {
	void test_thread_pool_executor_name();
	void test_thread_pool_executor_enqueue();

	void test_thread_pool_executor_destructor_test_0();
	void test_thread_pool_executor_destructor_test_1();
	void test_thread_pool_executor_destructor();
}

void concurrencpp::tests::test_thread_pool_executor_name() {
	auto executor = concurrencpp::make_runtime()->thread_pool_executor();
	assert_same(executor->name(), concurrencpp::details::consts::k_thread_pool_executor_name);
}

void concurrencpp::tests::test_thread_pool_executor_enqueue() {
	object_observer observer;
	const size_t count = 10'000;

	concurrencpp::runtime_options opts;
	opts.max_cpu_threads = 10;
	opts.max_background_threads = 5;

	auto executor = concurrencpp::make_runtime(opts)->thread_pool_executor();

	for (size_t i = 0; i < count; i++) {
		executor->enqueue(observer.get_testing_stub(std::chrono::milliseconds(0)));
	}

	assert_true(observer.wait_execution_count(count, std::chrono::minutes(2)));
	assert_true(observer.wait_destruction_count(count, std::chrono::minutes(2)));

	auto execution_map = observer.get_execution_map();

	assert_same(execution_map.size(), opts.max_cpu_threads);
}

void concurrencpp::tests::test_thread_pool_executor_destructor_test_0() {
	concurrencpp::runtime_options opts;
	opts.max_cpu_threads = 1;
	opts.max_background_threads = 0;

	auto executor = concurrencpp::make_runtime(opts)->thread_pool_executor();
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
		concurrencpp::details::consts::k_thread_pool_executor_cancel_error_msg);
}

void concurrencpp::tests::test_thread_pool_executor_destructor_test_1() {
	waiter waiter;
	object_observer observer;
	concurrencpp::runtime_options opts;
	const auto thread_count = 4;
	const auto task_count = 128;

	opts.max_cpu_threads = thread_count;
	opts.max_background_threads = 0;

	auto executor = concurrencpp::make_runtime(opts)->thread_pool_executor();

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

void concurrencpp::tests::test_thread_pool_executor_destructor() {
	test_thread_pool_executor_destructor_test_0();
	test_thread_pool_executor_destructor_test_1();
}

void concurrencpp::tests::test_thread_pool_executor() {
	tester tester("thread_pool_executor test");

	tester.add_step("name", test_thread_pool_executor_name);
	tester.add_step("enqueue", test_thread_pool_executor_enqueue);
	tester.add_step("~thread_pool_executor", test_thread_pool_executor_destructor);

	tester.launch_test();
}
