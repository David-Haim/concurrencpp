#include "concurrencpp.h"
#include "../all_tests.h"

#include "executor_test_helpers.h"

#include "../../tester/tester.h"
#include "../../helpers/assertions.h"
#include "../../helpers/object_observer.h"

#include "../../concurrencpp/src/executors/constants.h"

namespace concurrencpp::tests {
	void test_inline_executor_name();

	void test_inline_executor_shutdown();

	void test_inline_executor_max_concurrency_level();

	void test_inline_executor_post();
	void test_inline_executor_submit();

	void test_inline_executor_bulk_post();
	void test_inline_executor_bulk_submit();
}

using concurrencpp::details::thread;

void concurrencpp::tests::test_inline_executor_name() {
	auto executor = std::make_shared<inline_executor>();
	executor_shutdowner shutdown(executor);

	assert_equal(executor->name, concurrencpp::details::consts::k_inline_executor_name);
}

void concurrencpp::tests::test_inline_executor_shutdown() {
	auto executor = std::make_shared<inline_executor>();
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

void concurrencpp::tests::test_inline_executor_max_concurrency_level() {
	auto executor = std::make_shared<inline_executor>();
	executor_shutdowner shutdown(executor);

	assert_equal(executor->max_concurrency_level(),
		concurrencpp::details::consts::k_inline_executor_max_concurrency_level);
}

void concurrencpp::tests::test_inline_executor_post() {
	object_observer observer;
	const size_t task_count = 1'000;
	auto executor = std::make_shared<inline_executor>();
	executor_shutdowner shutdown(executor);

	for (size_t i = 0; i < task_count; i++) {
		executor->post(observer.get_testing_stub());
	}

	assert_equal(observer.get_execution_count(), task_count);
	assert_equal(observer.get_destruction_count(), task_count);

	const auto execution_map = observer.get_execution_map();

	assert_equal(execution_map.size(), size_t(1));
	assert_equal(execution_map.begin()->first , thread::get_current_virtual_id());
}

void concurrencpp::tests::test_inline_executor_submit() {
	object_observer observer;
	const size_t task_count = 1'000;
	auto executor = std::make_shared<inline_executor>();
	executor_shutdowner shutdown(executor);

	std::vector<result<size_t>> results;
	results.resize(task_count);

	for (size_t i = 0; i < task_count; i++) {
		results[i] = executor->submit(observer.get_testing_stub(i));
	}

	assert_equal(observer.get_execution_count(), task_count);
	assert_equal(observer.get_destruction_count(), task_count);

	const auto execution_map = observer.get_execution_map();

	assert_equal(execution_map.size(), size_t(1));
	assert_equal(execution_map.begin()->first, thread::get_current_virtual_id());

	for (size_t i = 0; i < task_count; i++) {
		assert_equal(results[i].get(), size_t(i));
	}
}

void concurrencpp::tests::test_inline_executor_bulk_post(){
	object_observer observer;
	const size_t task_count = 1'000;
	auto executor = std::make_shared<inline_executor>();
	executor_shutdowner shutdown(executor);

	std::vector<testing_stub> stubs;
	stubs.reserve(task_count);

	for (size_t i = 0; i < task_count; i++) {
		stubs.emplace_back(observer.get_testing_stub());
	}

	executor->template bulk_post<testing_stub>(stubs);

	assert_equal(observer.get_execution_count(), task_count);
	assert_equal(observer.get_destruction_count(), task_count);

	const auto execution_map = observer.get_execution_map();

	assert_equal(execution_map.size(), size_t(1));
	assert_equal(execution_map.begin()->first, thread::get_current_virtual_id());
}

void concurrencpp::tests::test_inline_executor_bulk_submit(){
	object_observer observer;
	const size_t task_count = 1'000;
	auto executor = std::make_shared<inline_executor>();
	executor_shutdowner shutdown(executor);

	std::vector<value_testing_stub> stubs;
	stubs.reserve(task_count);

	for (size_t i = 0; i < task_count; i++) {
		stubs.emplace_back(observer.get_testing_stub(i));
	}

	auto results = executor->template bulk_submit<value_testing_stub>(stubs);

	assert_equal(observer.get_execution_count(), task_count);
	assert_equal(observer.get_destruction_count(), task_count);

	const auto execution_map = observer.get_execution_map();

	assert_equal(execution_map.size(), size_t(1));
	assert_equal(execution_map.begin()->first, thread::get_current_virtual_id());

	for (size_t i = 0; i < task_count; i++) {
		assert_equal(results[i].get(), i);
	}
}

void concurrencpp::tests::test_inline_executor() {
	tester tester("inline_executor test");

	tester.add_step("name", test_inline_executor_name);
	tester.add_step("shutdown", test_inline_executor_shutdown);
	tester.add_step("max_concurrency_level", test_inline_executor_max_concurrency_level);
	tester.add_step("post", test_inline_executor_post);
	tester.add_step("submit", test_inline_executor_submit);
	tester.add_step("bulk_post", test_inline_executor_bulk_post);
	tester.add_step("bulk_submit", test_inline_executor_bulk_submit);

	tester.launch_test();
}
