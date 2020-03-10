#include "concurrencpp.h"
#include "../all_tests.h"

#include "../../tester/tester.h"
#include "../../helpers/assertions.h"
#include "../../helpers/thread_helpers.h"
#include "../../helpers/object_observer.h"

#include "../executor_tests/executor_test_helpers.h"

#include "../../../concurrencpp/src/executors/constants.h"
#include "../../../concurrencpp/src/threads/manual_worker.h"

namespace concurrencpp::tests {
	void test_manual_worker_destructor_test_0();
	void test_manual_worker_destructor_test_1();
	void test_manual_worker_destructor();

	void test_manual_worker_enqueue();

	void test_manual_worker_wait_all();
	void test_manual_worker_timed_wait_all();

	void test_manual_worker_loop_once();
	void test_manual_worker_loop();

	void test_manual_worker_cancel_all_test_0();
	void test_manual_worker_cancel_all_test_1();
	void test_manual_worker_cancel_all();

	void test_manual_worker_wait_for_task_untimed();
	void test_manual_worker_wait_for_task_timed();
	void test_manual_worker_wait_for_task();

	void test_manual_worker_wait_all();
	void test_manual_worker_timed_wait_all();
}

using concurrencpp::details::manual_worker;
using namespace std::chrono;
using namespace concurrencpp::details::consts;

void concurrencpp::tests::test_manual_worker_destructor_test_0() {
	object_observer observer;
	std::exception_ptr eptr;

	{
		manual_worker worker(k_manual_executor_cancel_error_msg);
		worker.enqueue(cancellation_tester(eptr));
	}

	assert_cancelled_correctly<concurrencpp::errors::broken_task>(
		eptr,
		k_manual_executor_cancel_error_msg);
}

void concurrencpp::tests::test_manual_worker_destructor_test_1() {
	const size_t task_count = 1'024;
	object_observer observer;

	{
		manual_worker worker("");
		for (size_t i = 0; i < task_count; i++) {
			worker.enqueue(observer.get_testing_stub());
		}
	}

	assert_same(observer.get_cancellation_count(), task_count);
	assert_same(observer.get_destruction_count(), task_count);
}

void concurrencpp::tests::test_manual_worker_destructor() {
	test_manual_worker_destructor_test_0();
	test_manual_worker_destructor_test_1();
}

void concurrencpp::tests::test_manual_worker_enqueue() {
	const size_t count = 1'024;
	object_observer observer;
	manual_worker worker("");

	assert_same(worker.size(), 0);
	assert_true(worker.empty());

	for (size_t i = 0; i < count; i++) {
		worker.enqueue(observer.get_testing_stub());
		assert_same(worker.size(), 1 + i);
		assert_false(worker.empty());
	}

	//manual executor doesn't execute the tasks automatically, hence manual.
	assert_same(observer.get_execution_count(), 0);
	assert_same(observer.get_destruction_count(), 0);
}

void concurrencpp::tests::test_manual_worker_loop_once() {
	const size_t count = 512;
	object_observer observer;
	manual_worker worker("");

	for (size_t i = 0; i < 10; i++) {
		assert_false(worker.loop_once());
	}

	for (size_t i = 0; i < count; i++) {
		worker.enqueue(observer.get_testing_stub());
	}

	for (size_t i = 0; i < count; i++) {
		assert_same(worker.size(), count - i);
		assert_true(worker.loop_once());
		assert_same(observer.get_execution_count(), 1 + i);
		assert_same(observer.get_destruction_count(), 1 + i);
	}

	for (size_t i = 0; i < 100; i++) {
		assert_false(worker.loop_once());
	}

	assert_same(observer.get_execution_count(), count);
	assert_same(observer.get_destruction_count(), count);

	auto execution_map = observer.get_execution_map();

	assert_same(execution_map.size(), 1);
	assert_same(execution_map.begin()->first, std::this_thread::get_id());
	assert_same(execution_map.begin()->second, count);
}

void concurrencpp::tests::test_manual_worker_loop() {
	object_observer observer;
	manual_worker worker("");

	const size_t count = 300;
	const size_t loop_count = 32;
	const size_t cycles = count / loop_count;
	const size_t remaining = count % loop_count;

	for (size_t i = 0; i < 10; i++) {
		const auto executed = worker.loop(loop_count);
		assert_same(executed, 0);
	}

	for (size_t i = 0; i < count; i++) {
		worker.enqueue(observer.get_testing_stub());
	}

	for (size_t i = 0; i < cycles; i++) {
		const auto executed = worker.loop(loop_count);
		assert_same(executed, loop_count);
		assert_same(worker.size(), count - (i + 1) * loop_count);
	}

	const auto remaining_executed = worker.loop(loop_count);
	assert_same(remaining_executed, remaining);
	assert_same(worker.size(), 0);
	assert_true(worker.empty());

	for (size_t i = 0; i < 10; i++) {
		const auto executed = worker.loop(loop_count);
		assert_same(executed, 0);
	}

	assert_same(observer.get_execution_count(), count);
	assert_same(observer.get_destruction_count(), count);

	auto execution_map = observer.get_execution_map();

	assert_same(execution_map.size(), 1);
	assert_same(execution_map.begin()->first, std::this_thread::get_id());
	assert_same(execution_map.begin()->second, count);
}

void concurrencpp::tests::test_manual_worker_cancel_all_test_0() {
	struct exception_capture {

	private:
		std::exception_ptr& m_dest;

	public:
		exception_capture(std::exception_ptr& dest) noexcept : m_dest(dest) {}

		void operator()() noexcept {}
		void cancel(std::exception_ptr reason) noexcept { m_dest = reason; }
	};

	manual_worker worker("");
	std::exception_ptr exception;
	const std::string error_msg = "error 123";
	const auto error = std::make_exception_ptr(std::runtime_error(error_msg));

	worker.enqueue(exception_capture{ exception });
	worker.cancel_all(error);

	assert_true(static_cast<bool>(exception));

	try {
		std::rethrow_exception(exception);
	}
	catch (std::runtime_error re) {
		assert_same(error_msg, re.what());
	}
	catch (...) {
		assert_false(true);
	}
}

void concurrencpp::tests::test_manual_worker_cancel_all_test_1() {
	const size_t task_count = 1'024;
	object_observer observer;
	manual_worker worker("");

	for (size_t i = 0; i < task_count; i++) {
		worker.enqueue(observer.get_testing_stub());
	}

	worker.cancel_all(std::make_exception_ptr(std::runtime_error("error.")));

	assert_same(observer.get_execution_count(), 0);
	assert_same(observer.get_cancellation_count(), task_count);
	assert_same(observer.get_destruction_count(), task_count);
}

void concurrencpp::tests::test_manual_worker_cancel_all() {
	test_manual_worker_cancel_all_test_0();
	test_manual_worker_cancel_all_test_1();
}

void concurrencpp::tests::test_manual_worker_wait_for_task_untimed() {
	manual_worker worker("");
	const auto later = high_resolution_clock::now() + seconds(2);

	std::thread enqueuing_thread([&worker, later]() mutable {
		std::this_thread::sleep_until(later);
		worker.enqueue([] {});
	});

	worker.wait_for_task();
	assert_bigger_equal(high_resolution_clock::now(), later);

	enqueuing_thread.join();
}

void concurrencpp::tests::test_manual_worker_wait_for_task_timed() {
	manual_worker worker("");
	const auto waiting_time = milliseconds(200);

	for (size_t i = 0; i < 10; i++) {
		const auto before = high_resolution_clock::now();
		const auto task_found = worker.wait_for_task(waiting_time);
		const auto after = high_resolution_clock::now();
		const auto time_elapsed = duration_cast<std::chrono::milliseconds>(after - before);

		assert_false(task_found);
		assert_bigger_equal(time_elapsed, waiting_time);
	}

	const auto later = high_resolution_clock::now() + seconds(2);

	std::thread enqueuing_thread([&worker, later]() mutable {
		std::this_thread::sleep_until(later);
		worker.enqueue([] {});
	});

	const auto task_found = worker.wait_for_task(std::chrono::seconds(10));

	assert_true(task_found);
	assert_bigger_equal(high_resolution_clock::now(), later);

	enqueuing_thread.join();
}

void concurrencpp::tests::test_manual_worker_wait_for_task() {
	test_manual_worker_wait_for_task_untimed();
	test_manual_worker_wait_for_task_timed();
}

void concurrencpp::tests::test_manual_worker_wait_all() {
	const auto task_count = 128;
	object_observer observer;
	manual_worker worker("");

	//if no tasks are processed, return immediately
	{
		const auto deadline = high_resolution_clock::now() + milliseconds(10);
		worker.wait_all();
		assert_smaller_equal(high_resolution_clock::now(), deadline);
	}

	for (size_t i = 0; i < task_count; i++) {
		worker.enqueue(observer.get_testing_stub());
	}

	const auto later = high_resolution_clock::now() + seconds(3);
	std::thread unblocker([later, task_count, &worker]() mutable {
		std::this_thread::sleep_until(later);
		assert_same(worker.loop(task_count), task_count);
	});

	worker.wait_all();

	assert_bigger_equal(std::chrono::high_resolution_clock::now(), later);
	assert_same(observer.get_execution_count(), task_count);

	unblocker.join();
}

void concurrencpp::tests::test_manual_worker_timed_wait_all() {
	manual_worker worker("");

	//if no tasks are in the pool, return immediately
	{
		const auto deadline = high_resolution_clock::now() + milliseconds(10);
		worker.wait_all(std::chrono::milliseconds(100));
		assert_smaller_equal(high_resolution_clock::now(), deadline);
	}

	const auto task_count = 128;
	object_observer observer;

	for (size_t i = 0; i < task_count; i++) {
		worker.enqueue(observer.get_testing_stub());
	}

	for (size_t i = 0; i < 5; i++) {
		assert_false(worker.wait_all(milliseconds(100)));
	}

	const auto later = high_resolution_clock::now() + seconds(2);
	std::thread executor([later, task_count, &worker]() mutable {
		std::this_thread::sleep_until(later);
		assert_same(worker.loop(task_count), task_count);
	});

	assert_true(worker.wait_all(seconds(20)));
	assert_same(observer.get_execution_count(), task_count);

	executor.join();
}

void concurrencpp::tests::test_manual_worker() {
	tester tester("manual_worker test");

	tester.add_step("~manual_worker", test_manual_worker_destructor);
	tester.add_step("enqueue", test_manual_worker_enqueue);
	tester.add_step("loop_once", test_manual_worker_loop_once);
	tester.add_step("loop", test_manual_worker_loop);
	tester.add_step("cancel_all", test_manual_worker_cancel_all);
	tester.add_step("wait_for_task", test_manual_worker_wait_for_task_untimed);
	tester.add_step("wait_for_task(ms)", test_manual_worker_wait_for_task_timed);
	tester.add_step("wait_all", test_manual_worker_wait_all);
	tester.add_step("wait_all(ms)", test_manual_worker_timed_wait_all);

	tester.launch_test();
}