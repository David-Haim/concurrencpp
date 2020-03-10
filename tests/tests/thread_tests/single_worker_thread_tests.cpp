#include "concurrencpp.h"
#include "../all_tests.h"

#include "../../tester/tester.h"
#include "../../helpers/assertions.h"
#include "../../helpers/thread_helpers.h"
#include "../../helpers/object_observer.h"

#include "../executor_tests/executor_test_helpers.h"

#include "../../../concurrencpp/src/executors/constants.h"
#include "../../../concurrencpp/src/threads/single_worker_thread.h"

namespace concurrencpp::tests {
	void test_single_worker_thread_destructor_test_0();
	void test_single_worker_thread_destructor_test_1();
	void test_single_worker_thread_destructor();

	void test_single_worker_thread_enqueue();

	void test_single_worker_thread_wait_all();
	void test_single_worker_thread_timed_wait_all();
}

using concurrencpp::details::single_worker_thread;
using namespace std::chrono;

void concurrencpp::tests::test_single_worker_thread_destructor_test_0() {
	const auto task_count = 1'024;
	object_observer observer;

	{
		single_worker_thread worker("");
		waiter waiter;

		for (size_t i = 0; i < 4; i++) {
			worker.enqueue([waiter]() mutable {
				waiter.wait();
			});
		}

		for (size_t i = 0; i < task_count; i++) {
			worker.enqueue(observer.get_testing_stub());
		}

		waiter.notify_all();
	}

	assert_true(observer.wait_cancel_count(task_count, seconds(20)));
	assert_true(observer.wait_destruction_count(task_count, seconds(20)));
}

void concurrencpp::tests::test_single_worker_thread_destructor_test_1() {
	std::exception_ptr exception_ptr;
	const auto error_msg = concurrencpp::details::consts::k_worker_thread_executor_cancel_error_msg;

	{
		single_worker_thread worker(error_msg);
		worker.enqueue([] {
			std::this_thread::sleep_for(std::chrono::seconds(1));
		});

		worker.enqueue(cancellation_tester(exception_ptr));
	}

	assert_cancelled_correctly<concurrencpp::errors::broken_task>(exception_ptr, error_msg);

}

void concurrencpp::tests::test_single_worker_thread_destructor() {
	test_single_worker_thread_destructor_test_0();
	test_single_worker_thread_destructor_test_1();
}

void concurrencpp::tests::test_single_worker_thread_enqueue() {
	const auto task_count = 1'024;
	single_worker_thread worker("");
	object_observer observer;

	for (size_t i = 0; i < task_count; i++) {
		worker.enqueue(observer.get_testing_stub());
	}

	assert_true(observer.wait_execution_count(task_count, std::chrono::seconds(30)));
	assert_true(observer.wait_destruction_count(task_count, std::chrono::seconds(30)));

	assert_same(observer.get_execution_map().size(), 1);
	assert_not_same(observer.get_execution_map().begin()->first, std::this_thread::get_id());
}

void concurrencpp::tests::test_single_worker_thread_wait_all() {
	const auto task_count = 128;
	waiter waiter;
	object_observer observer;
	single_worker_thread worker("");

	//if no tasks are processed, return immediately
	{
		const auto deadline = high_resolution_clock::now() + milliseconds(10);
		worker.wait_all();
		assert_smaller_equal(high_resolution_clock::now(), deadline);
	}

	worker.enqueue([waiter]() mutable {
		waiter.wait();
		std::this_thread::sleep_for(std::chrono::milliseconds(150));
	});

	const auto later = high_resolution_clock::now() + seconds(3);
	std::thread unblocker([later, waiter]() mutable {
		std::this_thread::sleep_until(later);
		waiter.notify_all();
	});

	for (size_t i = 0; i < task_count; i++) {
		worker.enqueue(observer.get_testing_stub());
	}

	worker.wait_all();

	assert_bigger_equal(std::chrono::high_resolution_clock::now(), later);
	assert_same(observer.get_execution_count(), task_count);

	unblocker.join();
}

void concurrencpp::tests::test_single_worker_thread_timed_wait_all() {
	single_worker_thread worker("");

	//if no tasks are in the pool, return immediately
	{
		const auto deadline = high_resolution_clock::now() + milliseconds(10);
		worker.wait_all(std::chrono::milliseconds(100));
		assert_smaller_equal(high_resolution_clock::now(), deadline);
	}

	object_observer observer;
	waiter waiter;
	const auto task_count = 1'024;

	worker.enqueue([waiter]() mutable {
		waiter.wait();
		std::this_thread::sleep_for(milliseconds(150));
	});

	for (size_t i = 0; i < task_count; i++) {
		worker.enqueue(observer.get_testing_stub());
	}

	for (size_t i = 0; i < 5; i++) {
		assert_false(worker.wait_all(milliseconds(100)));
	}

	waiter.notify_all();

	assert_true(worker.wait_all(seconds(30)));
	assert_same(observer.get_execution_count(), task_count);
}

void concurrencpp::tests::test_single_worker_thread() {
	tester tester("single_worker_thread test");

	tester.add_step("~single_worker_thread", test_single_worker_thread_destructor);
	tester.add_step("enqueue", test_single_worker_thread_enqueue);
	tester.add_step("wait_all", test_single_worker_thread_wait_all);
	tester.add_step("wait_all(ms)", test_single_worker_thread_timed_wait_all);

	tester.launch_test();
}
