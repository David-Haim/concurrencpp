#include "concurrencpp.h"
#include "../all_tests.h"

#include "../../tester/tester.h"
#include "../../helpers/assertions.h"
#include "../../helpers/object_observer.h"

#include "executor_test_helpers.h"
#include "../../concurrencpp/src/executors/constants.h"

namespace concurrencpp::tests {
	void test_manual_executor_name();
	void test_manual_executor_enqueue();

	void test_manual_executor_loop_once();
	void test_manual_executor_loop();

	void test_manual_executor_cancel_all_test_0();
	void test_manual_executor_cancel_all_test_1();
	void test_manual_executor_cancel_all();
	
	void test_manual_executor_wait_for_task_untimed();
	void test_manual_executor_wait_for_task_timed();
	void test_manual_executor_wait_for_task();

	void test_manual_executor_destructor_test_0();
	void test_manual_executor_destructor_test_1();
	void test_manual_executor_destructor();
}

void concurrencpp::tests::test_manual_executor_name() {
	auto executor = concurrencpp::make_runtime()->manual_executor();
	assert_same(executor->name(), concurrencpp::details::consts::k_manual_executor_name);
}

void concurrencpp::tests::test_manual_executor_enqueue() {
	object_observer observer;
	const size_t count = 1'000;	
	auto executor = concurrencpp::make_runtime()->manual_executor();

	assert_same(executor->size(), 0);
	assert_true(executor->empty());

	for (size_t i = 0; i < count; i++) {
		executor->enqueue(observer.get_testing_stub());
		assert_same(executor->size(), 1 + i);
		assert_false(executor->empty());
	}

	//manual executor doesn't execute the tasks automatically, hence manual.
	assert_same(observer.get_execution_count(), 0);
	assert_same(observer.get_destruction_count(), 0);
}

void concurrencpp::tests::test_manual_executor_loop_once() {
	object_observer observer;
	auto executor = concurrencpp::make_runtime()->manual_executor();
	const size_t count = 300;

	for (size_t i = 0; i < 100; i++) {
		assert_false(executor->loop_once());
	}

	for (size_t i = 0; i < count; i++) {
		executor->enqueue(observer.get_testing_stub());
	}

	for (size_t i = 0; i < count; i++) {
		assert_same(executor->size(), count - i);
		assert_true(executor->loop_once());
		assert_same(observer.get_execution_count(), 1 + i);
		assert_same(observer.get_destruction_count(), 1 + i);
	}

	for (size_t i = 0; i < 100; i++) {
		assert_false(executor->loop_once());
	}

	assert_same(observer.get_execution_count(), count);
	assert_same(observer.get_destruction_count(), count);
	
	auto execution_map = observer.get_execution_map();

	assert_same(execution_map.size(), 1);
	assert_same(execution_map.begin()->first , std::this_thread::get_id());
	assert_same(execution_map.begin()->second, count);
}

void concurrencpp::tests::test_manual_executor_loop() {
	object_observer observer;
	auto executor = concurrencpp::make_runtime()->manual_executor();
	const size_t count = 300;
	const size_t loop_count = 32;
	const size_t cycles = count / loop_count;
	const size_t remaining = count % loop_count;

	for (size_t i = 0; i < 10; i++) {
		const auto executed = executor->loop(loop_count);
		assert_same(executed, 0);
	}

	for (size_t i = 0; i < count; i++) {
		executor->enqueue(observer.get_testing_stub());
	}

	for (size_t i = 0; i < cycles; i++) {
		const auto executed = executor->loop(loop_count);
		assert_same(executed, loop_count);
		assert_same(executor->size(), count - (i + 1) * loop_count);
	}

	const auto remaining_executed = executor->loop(loop_count);
	assert_same(remaining_executed, remaining);
	assert_same(executor->size(), 0);
	assert_true(executor->empty());

	for (size_t i = 0; i < 10; i++) {
		const auto executed = executor->loop(loop_count);
		assert_same(executed, 0);
	}

	assert_same(observer.get_execution_count(), count);
	assert_same(observer.get_destruction_count(), count);

	auto execution_map = observer.get_execution_map();

	assert_same(execution_map.size(), 1);
	assert_same(execution_map.begin()->first, std::this_thread::get_id());
	assert_same(execution_map.begin()->second, count);
}

void concurrencpp::tests::test_manual_executor_cancel_all_test_0() {
	struct dummy_task {

	private:
		std::exception_ptr& m_dest;

	public:
		dummy_task(std::exception_ptr& dest) noexcept : m_dest(dest){}
		dummy_task(dummy_task&&) noexcept = default;

		void operator()() noexcept {}

		void cancel(std::exception_ptr reason) noexcept { m_dest = reason;  }
	};

	std::exception_ptr exception;
	auto executor = concurrencpp::make_runtime()->manual_executor();
	
	executor->enqueue(dummy_task{ exception });
	executor->cancel_all(std::make_exception_ptr(std::runtime_error("error.")));

	assert_true(static_cast<bool>(exception));

	try {
		std::rethrow_exception(exception);
	}
	catch (std::runtime_error re) {
		assert_same(std::string("error."), re.what());
	}
	catch (...) {
		assert_false(true);
	}
}

void concurrencpp::tests::test_manual_executor_cancel_all_test_1() {
	object_observer observer;
	auto executor = concurrencpp::make_runtime()->manual_executor();
	const size_t count = 1'000;

	for (size_t i = 0; i < count; i++) {
		executor->enqueue(observer.get_testing_stub());
	}

	executor->cancel_all(std::make_exception_ptr(std::runtime_error("error.")));

	assert_same(observer.get_execution_count(), 0);
	assert_same(observer.get_cancellation_count(), count);
	assert_same(observer.get_destruction_count(), count);
}

void concurrencpp::tests::test_manual_executor_cancel_all() {
	test_manual_executor_cancel_all_test_0();
	test_manual_executor_cancel_all_test_1();
}

void concurrencpp::tests::test_manual_executor_wait_for_task_untimed() {
	auto executor = concurrencpp::make_runtime()->manual_executor();
	auto enqueuing_time = std::chrono::system_clock::now() + std::chrono::milliseconds(2'500);

	std::thread enqueuing_thread([executor, enqueuing_time]() mutable {
		std::this_thread::sleep_until(enqueuing_time);	
		executor->enqueue([] {});
	});

	executor->wait_for_task();
	assert_bigger_equal(std::chrono::system_clock::now() , enqueuing_time);

	enqueuing_thread.join();
}

void concurrencpp::tests::test_manual_executor_wait_for_task_timed() {
	auto executor = concurrencpp::make_runtime()->manual_executor();
	const auto waiting_time = std::chrono::milliseconds(200);

	for (size_t i = 0; i < 10; i++) {
		const auto before = std::chrono::system_clock::now();
		const auto task_found = executor->wait_for_task(waiting_time);
		const auto after = std::chrono::system_clock::now();
		const auto time_elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(after - before);

		assert_false(task_found);
		assert_bigger_equal(time_elapsed , waiting_time);
	}

	auto enqueuing_time = std::chrono::system_clock::now() + std::chrono::milliseconds(2'500);

	std::thread enqueuing_thread([executor, enqueuing_time]() mutable {
		std::this_thread::sleep_until(enqueuing_time);
		executor->enqueue([] {});
	});

	const auto task_found = executor->wait_for_task(std::chrono::seconds(10));
	assert_true(task_found);
	assert_bigger_equal(std::chrono::system_clock::now() , enqueuing_time);

	enqueuing_thread.join();
}

void concurrencpp::tests::test_manual_executor_wait_for_task() {
	test_manual_executor_wait_for_task_untimed();
	test_manual_executor_wait_for_task_timed();
}

void concurrencpp::tests::test_manual_executor_destructor_test_0() {
	object_observer observer;
	auto executor = concurrencpp::make_runtime()->manual_executor();
	std::exception_ptr eptr;

	executor->enqueue(cancellation_tester(eptr));
	executor.reset();

	assert_cancelled_correctly<concurrencpp::errors::broken_task>(
		eptr,
		concurrencpp::details::consts::k_manual_executor_cancel_error_msg);
}

void concurrencpp::tests::test_manual_executor_destructor_test_1() {
	auto executor = concurrencpp::make_runtime()->manual_executor();
	const size_t task_count = 1'000;
	object_observer observer;

	for (size_t i = 0; i < task_count; i++) {
		executor->enqueue(observer.get_testing_stub());
	}

	executor.reset();

	assert_same(observer.get_cancellation_count(), task_count);
	assert_same(observer.get_destruction_count(), task_count);
}

void concurrencpp::tests::test_manual_executor_destructor() {
	test_manual_executor_destructor_test_0();
}

void concurrencpp::tests::test_manual_executor() {
	tester tester("manual_executor test");

	tester.add_step("name", test_manual_executor_name);
	tester.add_step("enqueue", test_manual_executor_enqueue);
	tester.add_step("loop_once", test_manual_executor_loop_once);
	tester.add_step("loop", test_manual_executor_loop);
	tester.add_step("cancel_all", test_manual_executor_cancel_all);
	tester.add_step("wait_for_task", test_manual_executor_wait_for_task);
	tester.add_step("~manual_executor", test_manual_executor_destructor);

	tester.launch_test();
}
