#include "concurrencpp.h"
#include "executor_test_helpers.h"

#include "../all_tests.h"

#include "../../tester/tester.h"
#include "../../helpers/assertions.h"
#include "../../helpers/object_observer.h"

#include "../../concurrencpp/src/executors/constants.h"

namespace concurrencpp::tests {
	void test_worker_thread_executor_name();

	void test_worker_thread_executor_shutdown_enqueue();
	void test_worker_thread_executor_shutdown_thread_join();
	void test_worker_thread_executor_shutdown_coro_raii();
	void test_worker_thread_executor_shutdown();

	void test_worker_thread_executor_max_concurrency_level();

	void test_worker_thread_executor_post();
	void test_worker_thread_executor_submit();

	void test_worker_thread_executor_bulk_post();
	void test_worker_thread_executor_bulk_submit();
}

using concurrencpp::details::thread;

void concurrencpp::tests::test_worker_thread_executor_name() {
	auto executor = std::make_shared<worker_thread_executor>();
	executor_shutdowner shutdown(executor);

	assert_equal(executor->name, concurrencpp::details::consts::k_worker_thread_executor_name);
}

void concurrencpp::tests::test_worker_thread_executor_shutdown_enqueue() {
	auto executor = std::make_shared<worker_thread_executor>();
	assert_false(executor->shutdown_requested());

	executor->shutdown();
	assert_true(executor->shutdown_requested());

	assert_throws<concurrencpp::errors::executor_shutdown>([executor] {
		executor->enqueue(std::experimental::coroutine_handle{});
	});

	assert_throws<concurrencpp::errors::executor_shutdown>([executor] {
		std::experimental::coroutine_handle<> array[4];
		std::span<std::experimental::coroutine_handle<>> span = array;
		executor->enqueue(span);
	});
}

void concurrencpp::tests::test_worker_thread_executor_shutdown_thread_join() {
	object_observer observer;
	auto executor = std::make_shared<worker_thread_executor>();

	executor->post([] {
		std::this_thread::sleep_for(std::chrono::seconds(1));
	});

	std::this_thread::sleep_for(std::chrono::milliseconds(150));

	executor->shutdown();
	assert_true(executor->shutdown_requested());
}

void concurrencpp::tests::test_worker_thread_executor_shutdown_coro_raii() {
	object_observer observer;
	const size_t task_count = 1'024;
	auto executor = std::make_shared<worker_thread_executor>();

	std::vector<value_testing_stub> stubs;
	stubs.reserve(task_count);

	for (size_t i = 0; i < 1'024; i++) {
		stubs.emplace_back(observer.get_testing_stub(i));
	}

	executor->post([] {
		std::this_thread::sleep_for(std::chrono::seconds(2));
	});

	auto results = executor->template bulk_submit<value_testing_stub>(stubs);

	executor->shutdown();
	assert_true(executor->shutdown_requested());

	assert_equal(observer.get_execution_count(), size_t(0));
	assert_equal(observer.get_destruction_count(), task_count);

	for (auto& result : results) {
		assert_throws<concurrencpp::errors::broken_task>([&result] {
			result.get();
		});
	}
}

void concurrencpp::tests::test_worker_thread_executor_shutdown() {
	test_worker_thread_executor_shutdown_enqueue();
	test_worker_thread_executor_shutdown_coro_raii();
	test_worker_thread_executor_shutdown_thread_join();
}

void concurrencpp::tests::test_worker_thread_executor_max_concurrency_level() {
	auto executor = std::make_shared<worker_thread_executor>();
	executor_shutdowner shutdown(executor);

	assert_equal(executor->max_concurrency_level(), 1);
}

void concurrencpp::tests::test_worker_thread_executor_post() {
	object_observer observer;
	const size_t task_count = 1'000;
	auto executor = std::make_shared<worker_thread_executor>();
	executor_shutdowner shutdown(executor);

	for (size_t i = 0; i < task_count; i++) {
		executor->post(observer.get_testing_stub());
	}

	assert_true(observer.wait_execution_count(task_count, std::chrono::minutes(1)));
	assert_true(observer.wait_destruction_count(task_count, std::chrono::minutes(1)));

	const auto& execution_map = observer.get_execution_map();

	assert_equal(execution_map.size(), size_t(1));
	assert_not_equal(execution_map.begin()->first, thread::get_current_virtual_id());
}

void concurrencpp::tests::test_worker_thread_executor_submit() {
	object_observer observer;
	const size_t task_count = 1'000;
	auto executor = std::make_shared<worker_thread_executor>();
	executor_shutdowner shutdown(executor);

	std::vector<result<size_t>> results;
	results.resize(task_count);

	for (size_t i = 0; i < task_count; i++) {
		results[i] = executor->submit(observer.get_testing_stub(i));
	}

	assert_true(observer.wait_execution_count(task_count, std::chrono::minutes(1)));
	assert_true(observer.wait_destruction_count(task_count, std::chrono::minutes(1)));

	const auto& execution_map = observer.get_execution_map();

	assert_equal(execution_map.size(), size_t(1));
	assert_not_equal(execution_map.begin()->first, thread::get_current_virtual_id());

	for (size_t i = 0; i < task_count; i++) {
		assert_equal(results[i].get(), i);
	}
}

void concurrencpp::tests::test_worker_thread_executor_bulk_post() {
	object_observer observer;
	const size_t task_count = 1'000;
	auto executor = std::make_shared<worker_thread_executor>();
	executor_shutdowner shutdown(executor);

	std::vector<testing_stub> stubs;
	stubs.reserve(task_count);

	for (size_t i = 0; i < task_count; i++) {
		stubs.emplace_back(observer.get_testing_stub());
	}

	executor->template bulk_post<testing_stub>(stubs);

	assert_true(observer.wait_execution_count(task_count, std::chrono::minutes(1)));
	assert_true(observer.wait_destruction_count(task_count, std::chrono::minutes(1)));

	const auto& execution_map = observer.get_execution_map();

	assert_equal(execution_map.size(), size_t(1));
	assert_not_equal(execution_map.begin()->first, thread::get_current_virtual_id());
}

void concurrencpp::tests::test_worker_thread_executor_bulk_submit() {
	object_observer observer;
	const size_t task_count = 1'000;
	auto executor = std::make_shared<worker_thread_executor>();
	executor_shutdowner shutdown(executor);

	std::vector<value_testing_stub> stubs;
	stubs.reserve(task_count);

	for (size_t i = 0; i < task_count; i++) {
		stubs.emplace_back(observer.get_testing_stub(i));
	}

	auto results = executor->template bulk_submit<value_testing_stub>(stubs);

	assert_true(observer.wait_execution_count(task_count, std::chrono::minutes(1)));
	assert_true(observer.wait_destruction_count(task_count, std::chrono::minutes(1)));

	const auto& execution_map = observer.get_execution_map();

	assert_equal(execution_map.size(), size_t(1));
	assert_not_equal(execution_map.begin()->first, thread::get_current_virtual_id());

	for (size_t i = 0; i < task_count; i++) {
		assert_equal(results[i].get(), i);
	}
}

void concurrencpp::tests::test_worker_thread_executor() {
	tester tester("worker_thread_executor test");

	tester.add_step("name", test_worker_thread_executor_name);
	tester.add_step("shutdown", test_worker_thread_executor_shutdown);
	tester.add_step("max_concurreny_level", test_worker_thread_executor_max_concurrency_level);
	tester.add_step("post", test_worker_thread_executor_post);
	tester.add_step("submit", test_worker_thread_executor_submit);
	tester.add_step("bulk_post", test_worker_thread_executor_bulk_post);
	tester.add_step("bulk_submit", test_worker_thread_executor_bulk_submit);

	tester.launch_test();
}
