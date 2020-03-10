#include "concurrencpp.h"
#include "../all_tests.h"

#include "../../tester/tester.h"
#include "../../helpers/assertions.h"
#include "../../helpers/thread_helpers.h"
#include "../../helpers/object_observer.h"

#include "../../../concurrencpp/details/threads/thread_group.h"

namespace concurrencpp::tests {
	void test_thread_group_enqueue_worker();
	void test_thread_group_destructor();
}

using concurrencpp::details::thread_group;

void concurrencpp::tests::test_thread_group_enqueue_worker() {
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

void concurrencpp::tests::test_thread_group() {
	tester tester("thread_group test");
	tester.add_step("enqueue_worker", test_thread_group_enqueue_worker);
	tester.add_step("destructor", test_thread_group_destructor);

	tester.launch_test();
}