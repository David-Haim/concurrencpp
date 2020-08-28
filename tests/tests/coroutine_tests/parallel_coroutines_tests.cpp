#include "concurrencpp.h"
#include "../all_tests.h"

#include "../result_tests/result_test_helpers.h"

#include "../../tester/tester.h"
#include "../../helpers/assertions.h"
#include "../../helpers/random.h"
#include "../../helpers/object_observer.h"

namespace concurrencpp::tests {
	template<class type>
	void test_parallel_coroutine_result_value_impl();
	void test_parallel_coroutine_result_value_void();
	void test_parallel_coroutine_result_value();

	template<class type>
	void test_parallel_coroutine_result_exception_impl();
	void test_parallel_coroutine_result_exception_void();
	void test_parallel_coroutine_result_exception();

	void test_parallel_coroutine_null_result_value();
	void test_parallel_coroutine_null_result_exception();
}

template<class type>
void concurrencpp::tests::test_parallel_coroutine_result_value_impl() {
	runtime runtime;

	constexpr size_t range_count = 1'024;
	auto range = result_factory<type>::get_many(range_count);
	std::vector<result<type>> results(range_count);

	for (size_t i = 0 ; i < range_count; i++){
		results[i] = [](concurrencpp::executor_tag, std::shared_ptr<thread_pool_executor>, type o)
			-> result<type> {
			co_return std::ref(o);
		}({}, runtime.thread_pool_executor(), range[i]);
	}

	for (size_t i = 0; i < range_count; i++) {
		results[i].wait();
		test_ready_result_result<type>(std::move(results[i]), range[i]);
	}
}

void concurrencpp::tests::test_parallel_coroutine_result_value_void() {
	concurrencpp::runtime runtime;

	constexpr size_t range_count = 1'024;
	std::vector<result<void>> results(range_count);

	for (size_t i = 0; i < range_count; i++) {
		results[i] = [](concurrencpp::executor_tag, std::shared_ptr<thread_pool_executor>)
			-> result<void> {
			co_return;
		}({}, runtime.thread_pool_executor());
	}

	for (size_t i = 0; i < range_count; i++) {
		results[i].wait();
		test_ready_result_result<void>(std::move(results[i]));
	}
}

void concurrencpp::tests::test_parallel_coroutine_result_value() {
	test_parallel_coroutine_result_value_impl<int>();
	test_parallel_coroutine_result_value_impl<std::string>();
	test_parallel_coroutine_result_value_impl<int&>();
	test_parallel_coroutine_result_value_impl<std::string&>();
	test_parallel_coroutine_result_value_void();
}

template <class type>
void concurrencpp::tests::test_parallel_coroutine_result_exception_impl() {
	concurrencpp::runtime runtime;

	constexpr size_t range_count = 1'024;
	auto range = result_factory<type>::get_many(range_count);
	std::vector<result<type>> results(range_count);

	for (size_t i = 0; i < range_count; i++) {
		results[i] = [](concurrencpp::executor_tag, std::shared_ptr<thread_pool_executor>, type o, size_t id)
			-> result<type> {
			throw costume_exception(id);
			co_return std::ref(o);
		}({}, runtime.thread_pool_executor(), range[i], i);
	}

	for (size_t i = 0; i < range_count; i++) {
		results[i].wait();
		test_ready_result_costume_exception(std::move(results[i]), i);
	}
}

void concurrencpp::tests::test_parallel_coroutine_result_exception_void() {
	concurrencpp::runtime runtime;

	constexpr size_t range_count = 1'024;
	std::vector<result<void>> results(range_count);

	for (size_t i = 0; i < range_count; i++) {
		results[i] = [](concurrencpp::executor_tag, std::shared_ptr<thread_pool_executor>, size_t id)
			-> result<void> {
			throw costume_exception(id);
			co_return;
		}({}, runtime.thread_pool_executor(), i);
	}

	for (size_t i = 0; i < range_count; i++) {
		results[i].wait();
		test_ready_result_costume_exception(std::move(results[i]), i);
	}
}

void concurrencpp::tests::test_parallel_coroutine_result_exception() {
	test_parallel_coroutine_result_exception_impl<int>();
	test_parallel_coroutine_result_exception_impl<std::string>();
	test_parallel_coroutine_result_exception_impl<int&>();
	test_parallel_coroutine_result_exception_impl<std::string&>();
	test_parallel_coroutine_result_exception_void();
}

void concurrencpp::tests::test_parallel_coroutine_null_result_value() {
	concurrencpp::runtime runtime;
	object_observer observer;

	constexpr size_t range_count = 1'024;

	for (size_t i = 0; i < range_count; i++) {
		[](concurrencpp::executor_tag, std::shared_ptr<thread_pool_executor>, testing_stub stub)
			-> null_result {
			stub();
			co_return;
		}({}, runtime.thread_pool_executor(), observer.get_testing_stub());
	}

	assert_true(observer.wait_execution_count(range_count, std::chrono::minutes(1)));
	assert_true(observer.wait_destruction_count(range_count, std::chrono::minutes(1)));
}

void concurrencpp::tests::test_parallel_coroutine_null_result_exception() {
	concurrencpp::runtime runtime;
	object_observer observer;

	constexpr size_t range_count = 1'024;

	for (size_t i = 0; i < range_count; i++) {
		[](concurrencpp::executor_tag, std::shared_ptr<thread_pool_executor>, testing_stub stub)
			-> null_result {
			stub();
			throw std::exception();
			co_return;
		}({}, runtime.thread_pool_executor(), observer.get_testing_stub());
	}

	assert_true(observer.wait_execution_count(range_count, std::chrono::minutes(1)));
	assert_true(observer.wait_destruction_count(range_count, std::chrono::minutes(1)));
}

void concurrencpp::tests::test_parallel_coroutine() {
	tester tester("parallel coroutine test");

	tester.add_step("result<type> - value", test_parallel_coroutine_result_value);
	tester.add_step("result<type> - exception", test_parallel_coroutine_result_exception);
	tester.add_step("null_result - value", test_parallel_coroutine_null_result_value);
	tester.add_step("null_result - exception", test_parallel_coroutine_null_result_exception);

	tester.launch_test();
}
