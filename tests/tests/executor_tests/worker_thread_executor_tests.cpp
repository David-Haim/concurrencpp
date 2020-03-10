#include "concurrencpp.h"
#include "../all_tests.h"

#include "../../tester/tester.h"
#include "../../helpers/assertions.h"
#include "../../helpers/object_observer.h"
#include "../../helpers/thread_helpers.h"

#include "executor_test_helpers.h"
#include "../../concurrencpp/details/executors/constants.h"

namespace concurrencpp::tests {
	void test_worker_thread_executor_name();
	void test_worker_thread_executor_enqueue();

	void test_worker_thread_executor_destructor_test_0();
	void test_worker_thread_executor_destructor_test_1();
	void test_worker_thread_executor_destructor();
}

void concurrencpp::tests::test_worker_thread_executor_name() {
	auto executor = concurrencpp::make_runtime()->worker_thread_executor();
	assert_same(executor->name(), concurrencpp::details::consts::k_worker_thread_executor_name);
}

void concurrencpp::tests::test_worker_thread_executor_enqueue() {
	object_observer observer;
	const size_t count = 1'000;
	auto executor = concurrencpp::make_runtime()->worker_thread_executor();

	for (size_t i = 0; i < count; i++) {
		executor->enqueue(observer.get_testing_stub());
	}

	assert_true(observer.wait_execution_count(count, std::chrono::minutes(2)));
	assert_true(observer.wait_destruction_count(count, std::chrono::minutes(2)));

	auto execution_map = observer.get_execution_map();

	assert_same(execution_map.size(), 1);
	assert_not_same(execution_map.begin()->first, std::this_thread::get_id());
}

void concurrencpp::tests::test_worker_thread_executor_destructor_test_0() {
	waiter waiter;
	object_observer observer;
	auto executor = concurrencpp::make_runtime()->worker_thread_executor();
	std::exception_ptr exception_ptr;

	executor->enqueue([waiter]() mutable {
		waiter.wait();
	});

	executor->enqueue(cancellation_tester(exception_ptr));
	
	waiter.notify_all();
	executor.reset();

	assert_cancelled_correctly<concurrencpp::errors::broken_task>(
		exception_ptr,
		concurrencpp::details::consts::k_worker_thread_executor_cancel_error_msg);
}

void concurrencpp::tests::test_worker_thread_executor_destructor_test_1() {
	waiter waiter;
	object_observer observer;
	auto executor = concurrencpp::make_runtime()->worker_thread_executor();
	const auto task_count = 1'000;

	executor->enqueue([waiter]() mutable {
		waiter.wait();
	});

	for (size_t i = 0; i < task_count; i++) {
		executor->enqueue(observer.get_testing_stub());
	}

	waiter.notify_all();
	executor.reset();

	//worker_thread destructor returns after all tasks are cancelled.
	assert_same(observer.get_execution_count(), 0);
	assert_same(observer.get_cancellation_count(), task_count);
	assert_same(observer.get_destruction_count(), task_count);
}

void concurrencpp::tests::test_worker_thread_executor_destructor() {
	test_worker_thread_executor_destructor_test_0();
	test_worker_thread_executor_destructor_test_1();
}

void concurrencpp::tests::test_worker_thread_executor() {
	tester tester("worker_thread_executor test");

	tester.add_step("name", test_worker_thread_executor_name);
	tester.add_step("enqueue", test_worker_thread_executor_enqueue);
	tester.add_step("~worker_thread_executor", test_worker_thread_executor_destructor);

	tester.launch_test();
}
