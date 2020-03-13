#include "concurrencpp.h"
#include "../all_tests.h"

#include "../../tester/tester.h"
#include "../../helpers/assertions.h"
#include "../../helpers/thread_helpers.h"
#include "../../helpers/fast_randomizer.h"
#include "../../helpers/object_observer.h"

#include "../details/executors/constants.h"
#include "../details/threads/thread_pool.h"
#include "../executor_tests/executor_test_helpers.h"

#include <mutex>
#include <algorithm>
#include <unordered_set>

#include <iostream>

namespace concurrencpp::tests {
	void test_thread_pool_enqueue_case_1();
	void test_thread_pool_enqueue_case_1_not_tp_thread();
	void test_thread_pool_enqueue_case_1_task_queue_not_empty();
	
	void test_thread_pool_enqueue_case_2();
	void test_thread_pool_enqueue_case_2_tp_thread();

	void test_thread_pool_enqueue_case_3();
	void test_thread_pool_enqueue_case_4();

	void test_thread_pool_enqueue();

	void test_thread_pool_dynamic_resizing_test_1();
	void test_thread_pool_dynamic_resizing_test_2();
	void test_thread_pool_dynamic_resizing();
	void test_thread_pool_work_stealing();

	void test_thread_pool_destructor_cancellation();
	void test_thread_pool_destructor_pending_tasks();
	void test_thread_pool_destructor_joining_threads();
	void test_thread_pool_destructor();

	void test_thread_pool_combo();
}

using concurrencpp::details::thread_pool;
using concurrencpp::details::thread_group;
using namespace std::chrono;

void concurrencpp::tests::test_thread_pool_enqueue_case_1() {
	/*
		Case 1: if this thread is a threadpool thread and its task queue is empty
				then enqueue to this thread
	*/
	waiter waiter;
	thread_pool pool(4, seconds(20), "", nullptr);

	pool.enqueue([&]() mutable {
		//we are inside a threadpool thread;
		const auto chosen_id_0 = pool.enqueue([] {}); //this_thread is a tp thread and it's task quue is empty.
		assert_same(chosen_id_0, std::this_thread::get_id());
		waiter.notify_all();
	});

	waiter.wait();
}

void concurrencpp::tests::test_thread_pool_enqueue_case_1_not_tp_thread() {
	thread_pool pool(4, seconds(20), "", nullptr);
	const auto id = pool.enqueue([] {});
	assert_not_same(id, std::this_thread::get_id());
}

void concurrencpp::tests::test_thread_pool_enqueue_case_1_task_queue_not_empty() {
	waiter waiter;
	thread_pool pool(4, seconds(20), "", nullptr);

	pool.enqueue([&]() mutable {
		//we are inside a threadpool thread;
		const auto chosen_id_0 = pool.enqueue([] {}); //this_thread is a tp thread and it's task quue is empty.
		assert_same(chosen_id_0, std::this_thread::get_id());

		const auto chosen_id_1 = pool.enqueue([] {});
		assert_not_same(chosen_id_1, std::this_thread::get_id());
		waiter.notify_all();
	});

	waiter.wait();
}

void concurrencpp::tests::test_thread_pool_enqueue_case_2() {
	/*
		Case 2: if case 1 is false and a waiting thread exist
				then enqueue to that thread

		this test a non tp thread -> should use a waiting thread, if exists
	*/
	waiter waiter;
	thread_pool pool(4, seconds(20), "", nullptr);
		
	for (size_t i = 0; i < 2; i++) {
		pool.enqueue([waiter]() mutable {
			waiter.wait();
		});
	}

	std::this_thread::sleep_for(seconds(2));

	const auto waiting_thread_id = pool.enqueue([] {});

	std::this_thread::sleep_for(seconds(2));

	for (size_t i = 0; i < 10; i++) {
		const auto id = pool.enqueue([] {});
		assert_same(id, waiting_thread_id);

		std::this_thread::sleep_for(seconds(1));
	}

	waiter.notify_all();
}

void concurrencpp::tests::test_thread_pool_enqueue_case_2_tp_thread() {
	/*
		this test tests case 2 but in the case that this_thread is a thread pool thread
		but its task queue is not empty. in this case if a waiting thread exists, it should enqueue
		the task to that waiting thread.
	*/

	thread_pool pool(4, seconds(20), "", nullptr);
	waiter waiter;

	std::thread::id running_threads[2];
	std::thread::id tested_threads[2];

	for (size_t i = 0; i < 2; i++) {
		running_threads[i] = pool.enqueue([waiter]() mutable {
			waiter.wait();
		});
	}

	assert_not_same(running_threads[0], running_threads[1]);

	for (size_t i = 0; i < 2; i++) {
		tested_threads[i] = pool.enqueue([] {
			std::this_thread::sleep_for(milliseconds(500));
		});
	}

	assert_not_same(tested_threads[0], tested_threads[1]);

	std::this_thread::sleep_for(seconds(2));

	pool.enqueue([
		&pool,
		running_thread_0 = running_threads[0],
		running_thread_1 = running_threads[1],
		tested_thread_0 = tested_threads[0],
		tested_thread_1 = tested_threads[1]
	]() mutable {	
		const auto this_thread = pool.enqueue([] {});
		assert_same(this_thread, std::this_thread::get_id());
		assert_true(this_thread == tested_thread_0 || this_thread == tested_thread_1);
		assert_false(this_thread == running_thread_0 || this_thread == running_thread_1);

		const auto waiting_thread = pool.enqueue([] {});
		assert_not_same(waiting_thread, this_thread);
		assert_true(waiting_thread == tested_thread_0 || waiting_thread == tested_thread_1);
		assert_false(waiting_thread == running_thread_0 || waiting_thread == running_thread_1);
	});

	std::this_thread::sleep_for(seconds(2));
	waiter.notify_all();
}

void concurrencpp::tests::test_thread_pool_enqueue_case_3() {
	/*
		Case 3: if case 2 is false and this thread is a thread pool thread 
				then enqueue to this thread
	*/

	thread_pool pool(4, seconds(20), "", nullptr);
	waiter waiter;

	for (size_t i = 0; i < 3; i++) {
		pool.enqueue([waiter]() mutable {
			waiter.wait();
		});
	}

	pool.enqueue([&] {
		for (size_t i = 0; i < 64; i++) {
			const auto thread = pool.enqueue([] {});
			assert_same(thread, std::this_thread::get_id());
		}
	});

	std::this_thread::sleep_for(seconds(2));

	waiter.notify_all();
}

void concurrencpp::tests::test_thread_pool_enqueue_case_4() {
	/*
		Case 4: if case 3 is false then enqueue the task to a threadpool thread using round robin
	*/

	const auto thread_count = 4;
	const auto task_count = 4 * 6;
	thread_pool pool(thread_count, seconds(20), "", nullptr);
	waiter waiter;

	for (size_t i = 0; i < thread_count; i++) {
		pool.enqueue([waiter]() mutable {
			waiter.wait();
		});
	}

	std::unordered_map<std::thread::id, size_t> enqueueing_map;
	for (size_t i = 0; i < task_count; i++) {
		const auto id = pool.enqueue([] {});
		++enqueueing_map[id];
	}

	assert_same(enqueueing_map.size(), thread_count);
	for (const auto pair : enqueueing_map) {
		assert_same(pair.second, task_count / thread_count);
	}
	
	waiter.notify_all();
}
void concurrencpp::tests::test_thread_pool_enqueue() {
	test_thread_pool_enqueue_case_1();
	test_thread_pool_enqueue_case_1_not_tp_thread();
	test_thread_pool_enqueue_case_1_task_queue_not_empty();
	
	test_thread_pool_enqueue_case_2();
	test_thread_pool_enqueue_case_2_tp_thread();

	test_thread_pool_enqueue_case_3();
	test_thread_pool_enqueue_case_4();
}

void concurrencpp::tests::test_thread_pool_destructor_pending_tasks() {
	waiter waiter;
	object_observer observer;	
	auto pool = std::make_unique<thread_pool>(1, minutes(1), "", nullptr);

	pool->enqueue([waiter]() mutable {
		waiter.wait();
		std::this_thread::sleep_for(seconds(10));
	});

	for (size_t i = 0; i < 100; i++) {
		pool->enqueue(observer.get_testing_stub());
	}

	waiter.notify_all();
	pool.reset();

	assert_same(observer.get_cancellation_count(), 100);
	assert_same(observer.get_destruction_count(), 100);
}

void concurrencpp::tests::test_thread_pool_destructor_cancellation() {
	std::exception_ptr exception_ptr;
	const auto error_msg = concurrencpp::details::consts::k_thread_pool_executor_cancel_error_msg;

	{
		thread_pool pool(1, minutes(1), error_msg, nullptr);
		pool.enqueue([] {
			std::this_thread::sleep_for(std::chrono::seconds(2));
		});

		pool.enqueue(cancellation_tester(exception_ptr));
	}

	assert_cancelled_correctly<concurrencpp::errors::broken_task>(exception_ptr, error_msg);
}

void concurrencpp::tests::test_thread_pool_destructor_joining_threads() {
	auto listener = make_test_listener();

	{
		thread_pool pool(24, minutes(1), "", listener);
		for (size_t i = 0; i < 8; i++) {
			pool.enqueue([] { std::this_thread::sleep_for(seconds(10)); });
		}

		for (size_t i = 0; i < 8; i++) {
			pool.enqueue([] { std::this_thread::sleep_for(seconds(1)); });
		}

		std::this_thread::sleep_for(seconds(5));

		//third of the threads are working, third are waiting, third non existent.
		//all should quit and be destructed cleanly.
	}

	assert_same(listener->total_created(), 16);
	assert_same(listener->total_destroyed(), 16);
}

void concurrencpp::tests::test_thread_pool_destructor() {
	test_thread_pool_destructor_cancellation();
	test_thread_pool_destructor_pending_tasks();
	test_thread_pool_destructor_joining_threads();
}

void concurrencpp::tests::test_thread_pool_dynamic_resizing_test_1() {
	auto listener = make_test_listener();
	auto pool = std::make_unique<thread_pool>(1, seconds(8), "", listener);
	auto do_nothing = [] {};

	assert_same(listener->total_created(), 0);
	pool->enqueue(do_nothing);
	assert_same(listener->total_created(), 1);

	std::this_thread::sleep_for(seconds(1));

	assert_same(listener->total_waiting(), 1);
	pool->enqueue(do_nothing);

	std::this_thread::sleep_for(seconds(1));
	assert_same(listener->total_resuming(), 1);

	std::this_thread::sleep_for(seconds(12));
	assert_same(listener->total_idling(), 1);

	pool->enqueue(do_nothing);
	assert_same(listener->total_created(), 2); //one thread was created and went idle. another one was created instead.

	pool.reset();
	assert_same(listener->total_destroyed(), 2); 
}

void concurrencpp::tests::test_thread_pool_dynamic_resizing_test_2() {
	auto listener = make_test_listener();
	auto pool = std::make_unique<thread_pool>(10, seconds(5), "", listener);
	auto sleep_50 = [] {std::this_thread::sleep_for(milliseconds(50)); };
	auto sleep_500 = [] {std::this_thread::sleep_for(milliseconds(500)); };

	for (size_t i = 0; i < 10; i++) {
		pool->enqueue(sleep_50);
		assert_same(listener->total_created(), i + 1);
	}

	std::this_thread::sleep_for(seconds(3));
	assert_same(listener->total_waiting(), 10);

	for (size_t i = 0; i < 10; i++) {
		pool->enqueue(sleep_500);
	}

	std::this_thread::sleep_for(seconds(1));
	assert_same(listener->total_resuming(), 10);

	std::this_thread::sleep_for(seconds(10));
	assert_same(listener->total_idling(), 10);

	pool.reset();
	assert_same(listener->total_destroyed(), 10);
}

void concurrencpp::tests::test_thread_pool_dynamic_resizing() {	
	test_thread_pool_dynamic_resizing_test_1();
	test_thread_pool_dynamic_resizing_test_2();
}

void concurrencpp::tests::test_thread_pool_work_stealing() {
	auto listener = make_test_listener();
	waiter waiter;
	object_observer observer;
	thread_pool pool(4, seconds(20), "", listener);

	for (size_t i = 0; i < 3; i++) {
		pool.enqueue([waiter]() mutable { 
			waiter.wait(); 
		});
	}

	std::this_thread::sleep_for(seconds(2));

	const size_t task_count = 400;
	for (size_t i = 0; i < task_count; i++) {
		pool.enqueue(observer.get_testing_stub());
	}

	assert_true(observer.wait_execution_count(task_count, seconds(30)));
	assert_true(observer.wait_destruction_count(task_count, seconds(30)));

	assert_same(observer.get_execution_map().size(), 1);

	waiter.notify_all();
}

void concurrencpp::tests::test_thread_pool_combo() {
	auto listener = make_test_listener();
	object_observer observer;
	random randomizer;
	size_t expected_count = 0;	
	auto pool = std::make_unique<thread_pool>(32, seconds(4),"", listener);
	
	for (size_t i = 0; i < 50; i++) {
		const auto task_count = static_cast<size_t>(randomizer(0, 1'500));

		for (size_t j = 0; j < task_count; j++) {
			const auto sleeping_time = randomizer(0, 10);
			pool->enqueue(observer.get_testing_stub(milliseconds(sleeping_time)));
		}

		expected_count += task_count;

		const auto sleeping_time = randomizer(0, 5);
		std::this_thread::sleep_for(seconds(sleeping_time));
	}

	assert_true(observer.wait_execution_count(expected_count, seconds(60 * 2)));

	pool.reset();

	assert_same(observer.get_cancellation_count(), 0);
	assert_same(observer.get_destruction_count(), expected_count);
	assert_same(listener->total_created(), listener->total_destroyed());
}

void concurrencpp::tests::test_thread_pool() {
	tester tester("thread_pool test");

	tester.add_step("enqueue", test_thread_pool_enqueue);
	tester.add_step("thread pool work stealing", test_thread_pool_work_stealing);
	tester.add_step("threadpool auto resizing", test_thread_pool_dynamic_resizing); 
	tester.add_step("threadpool combo", test_thread_pool_combo);
	tester.add_step("~thread_pool", test_thread_pool_destructor);

	tester.launch_test();
}