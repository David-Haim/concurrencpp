#include "concurrencpp.h"
#include "../all_tests.h"

#include "../../tester/tester.h"
#include "../../helpers/assertions.h"
#include "../../helpers/thread_helpers.h"
#include "../../helpers/object_observer.h"

#include "../../../concurrencpp/src/threads/thread_group.h"

namespace concurrencpp::tests {
	void test_thread_group_enqueue();
	void test_thread_group_destructor();
	void test_thread_group_wait_all();
	void test_thread_group_timed_wait_all();
}

using concurrencpp::details::thread_group;

void concurrencpp::tests::test_thread_group_enqueue() {
	auto listener = make_test_listener();
	object_observer observer;
	const size_t task_count = 256;

	{
		thread_group group(listener);

		for (auto i = 0; i < task_count; i++) {
			group.enqueue(observer.get_testing_stub());
		}

		assert_true(observer.wait_execution_count(task_count, std::chrono::seconds(60 * 2)));
	}

	assert_same(observer.get_destruction_count(), task_count);
	assert_same(listener->total_created(), task_count);
	assert_same(listener->total_idling(), task_count);
}

void concurrencpp::tests::test_thread_group_destructor() {
	object_observer observer;
	auto listener = make_test_listener();
	const size_t task_count = 32;

	{
		thread_group group(listener);

		for (auto i = 0; i < task_count; i++) {
			group.enqueue(observer.get_testing_stub());
		}
	}

	assert_same(observer.get_execution_count(), task_count);
	assert_same(observer.get_destruction_count(), task_count);
	assert_same(listener->total_created(), task_count);
	assert_same(listener->total_idling(), task_count);
	assert_same(listener->total_destroyed(), task_count);
}

void concurrencpp::tests::test_thread_group_wait_all() {
	object_observer observer;
	waiter waiter;
	thread_group group(nullptr);

	//if no tasks are running, return immediately
	{
		const auto deadline = std::chrono::high_resolution_clock::now() + std::chrono::milliseconds(10);
		group.wait_all();
		assert_smaller_equal(std::chrono::high_resolution_clock::now(), deadline);
	}

	const auto later = std::chrono::high_resolution_clock::now() + std::chrono::seconds(3);
	std::thread unblocker([later, waiter]() mutable {
		std::this_thread::sleep_until(later);
		waiter.notify_all();
	});

	group.enqueue([waiter]() mutable {
		waiter.wait();
	});

	const auto task_count = 16;
	for (size_t i = 0; i < task_count; i++) {
		group.enqueue(observer.get_testing_stub());
	}

	group.wait_all();
	assert_bigger_equal(std::chrono::high_resolution_clock::now(), later);
	assert_same(observer.get_execution_count(), task_count);

	unblocker.join();
}

void concurrencpp::tests::test_thread_group_timed_wait_all() {
	object_observer observer;
	waiter waiter;
	thread_group group(nullptr);

	//if no tasks are running, return immediately
	{
		const auto deadline = std::chrono::high_resolution_clock::now() + std::chrono::milliseconds(10);
		group.wait_all(std::chrono::milliseconds(100));
		assert_smaller_equal(std::chrono::high_resolution_clock::now(), deadline);
	}

	group.enqueue([waiter]() mutable {
		waiter.wait();
		std::this_thread::sleep_for(std::chrono::milliseconds(150));
	});

	const auto task_count = 16;
	for (size_t i = 0; i < task_count; i++) {
		group.enqueue(observer.get_testing_stub());
	}

	for (size_t i = 0; i < 5; i++) {
		assert_false(group.wait_all(std::chrono::milliseconds(100)));
	}

	waiter.notify_all();

	assert_true(group.wait_all(std::chrono::seconds(10)));
	assert_same(observer.get_execution_count(), task_count);
}

void concurrencpp::tests::test_thread_group() {
	tester tester("thread_group test");

	tester.add_step("destructor", test_thread_group_destructor);
	tester.add_step("enqueue", test_thread_group_enqueue);
	tester.add_step("wait_all", test_thread_group_wait_all);
	tester.add_step("wait_all(ms)", test_thread_group_timed_wait_all);

	tester.launch_test();
}