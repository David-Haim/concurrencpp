#include "concurrencpp.h"

#include "../all_tests.h"

#include "../executor_tests/executor_test_helpers.h"
#include "../result_tests/result_test_helpers.h"

#include "../../tester/tester.h"
#include "../../helpers/assertions.h"
#include "../../helpers/object_observer.h"

#include <string>

namespace concurrencpp::tests {
	template<class type>
	result<type> sync_coro_val(testing_stub stub);

	template<class type>
	result<type> sync_coro_err(testing_stub stub);

	template<class type>
	result<type> async_coro_val(testing_stub stub);

	template<class type>
	result<type> async_coro_err(testing_stub stub);

	template<class type>
	void test_coro_RAII_sync();

	template<class type>
	void test_coro_RAII_async();

	template<class type>
	void test_coro_RAII_impl();
	void test_coro_RAII();

	template<class type>
	void test_ready_result_value_impl();

	template<class type>
	void test_ready_result_exception_impl();
	void test_ready_result();

	template<class type>
	void test_non_ready_result_value_impl();

	template<class type>
	void test_non_ready_result_exception_impl();
	void test_non_ready_result();

	template<class type>
	result<type> recursive_coro_value(const size_t cur_depth, const size_t max_depth);
	template<class type>
	result<type> recursive_coro_exception(const size_t cur_depth, const size_t max_depth);

	template<class type>
	void test_coroutines_recursivly_impl();
	void test_coroutines_recursivly();

	void test_coro_combo_value();
	void test_coro_combo_exception();
	void test_coro_combo();
}


namespace concurrencpp::tests::coroutines {
	std::shared_ptr<concurrencpp::thread_executor> get_coroutine_test_ex() {
		static const auto s_executor = std::make_shared<concurrencpp::thread_executor>();
		static executor_shutdowner shutdown(s_executor);
		return s_executor;
	}
}

using concurrencpp::result;
using concurrencpp::thread_pool_executor;
using concurrencpp::tests::object_observer;

template<class type>
result<type> concurrencpp::tests::sync_coro_val(testing_stub stub) {
	co_return result_factory<type>::get();
}

template<class type>
result<type> concurrencpp::tests::sync_coro_err(testing_stub stub) {
	co_return result_factory<type>::throw_ex();
}

template<class type>
result<type> concurrencpp::tests::async_coro_val(testing_stub stub) {
	auto res = coroutines::get_coroutine_test_ex()->submit([]() -> decltype(auto) {
		std::this_thread::sleep_for(std::chrono::milliseconds(15));
		return result_factory<type>::get();
	});

	co_return co_await res;
}

template<class type>
result<type> concurrencpp::tests::async_coro_err(testing_stub stub) {
	auto _stub = std::move(stub);
	auto res = coroutines::get_coroutine_test_ex()->submit([]() -> decltype(auto) {
		std::this_thread::sleep_for(std::chrono::milliseconds(15));
		return result_factory<type>::throw_ex();
	});

	co_return co_await res;
}

template<class type>
void concurrencpp::tests::test_coro_RAII_sync() {
	//value
	{
		object_observer observer;

		{
			auto result = sync_coro_val<type>(observer.get_testing_stub());
		}

		assert_equal(observer.get_destruction_count(), size_t(1));
	}

	//err
	{
		object_observer observer;

		{
			auto result = sync_coro_err<type>(observer.get_testing_stub());
		}

		assert_equal(observer.get_destruction_count(), size_t(1));
	}
}

template<class type>
void concurrencpp::tests::test_coro_RAII_async() {
	//result is dead before the coro - value
	{
		object_observer observer;

		{
			auto result = async_coro_val<type>(observer.get_testing_stub());
		}

		assert_true(observer.wait_destruction_count(1, std::chrono::seconds(30)));
	}

	//result is dead before the coro - err
	{
		object_observer observer;

		{
			auto result = async_coro_err<type>(observer.get_testing_stub());
		}

		assert_true(observer.wait_destruction_count(1, std::chrono::seconds(30)));
	}

	//coro is dead before the result - val
	{
		object_observer observer;
		auto result = async_coro_val<type>(observer.get_testing_stub());
		result.get();

		assert_true(observer.wait_destruction_count(1, std::chrono::seconds(30)));
	}

	//coro is dead before the result - err
	{
		object_observer observer;
		auto result = async_coro_err<type>(observer.get_testing_stub());

		try {
			result.get();
		}
		catch (...) {
			//do nothing
		}

		assert_true(observer.wait_destruction_count(1, std::chrono::seconds(30)));
	}
}

template<class type>
void concurrencpp::tests::test_coro_RAII_impl() {
	test_coro_RAII_sync<int>();
	test_coro_RAII_sync<std::string>();
	test_coro_RAII_sync<void>();
	test_coro_RAII_sync<int&>();
	test_coro_RAII_sync<std::string&>();

	test_coro_RAII_async<int>();
	test_coro_RAII_async<std::string>();
	test_coro_RAII_async<void>();
	test_coro_RAII_async<int&>();
	test_coro_RAII_async<std::string&>();
}

void concurrencpp::tests::test_coro_RAII() {
	test_coro_RAII_impl<int>();
	test_coro_RAII_impl<std::string>();
	test_coro_RAII_impl<void>();
	test_coro_RAII_impl<int&>();
	test_coro_RAII_impl<std::string&>();
}

template<class type>
void concurrencpp::tests::test_ready_result_value_impl() {
	test_ready_result_result([]() -> result<type> {
		co_return result_factory<type>::get();
	}());
}

template<class type>
void concurrencpp::tests::test_ready_result_exception_impl() {
	test_ready_result_costume_exception([]() -> result<type> {
		throw costume_exception(123456789);
		co_return result_factory<type>::get();
	}(), 123456789);
}

void concurrencpp::tests::test_ready_result() {
	test_ready_result_value_impl<int>();
	test_ready_result_value_impl<std::string>();
	test_ready_result_value_impl<void>();
	test_ready_result_value_impl<int&>();
	test_ready_result_value_impl<std::string&>();

	test_ready_result_exception_impl<int>();
	test_ready_result_exception_impl<std::string>();
	test_ready_result_exception_impl<void>();
	test_ready_result_exception_impl<int&>();
	test_ready_result_exception_impl<std::string&>();
}

template<class type>
void concurrencpp::tests::test_non_ready_result_value_impl() {
	auto result_0 = coroutines::get_coroutine_test_ex()->submit([]() -> decltype(auto) {
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
		return result_factory<type>::get();
	});

	auto coroutine = [result_0 = std::move(result_0)]() mutable->result<type> {
		co_return co_await result_0;
	};

	auto result_1 = coroutine();
	result_1.wait();
	test_ready_result_result(std::move(result_1));
}

template<class type>
void concurrencpp::tests::test_non_ready_result_exception_impl() {
	auto result_0 = coroutines::get_coroutine_test_ex()->submit([]() -> decltype(auto) {
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
		throw costume_exception(123456789);
		return result_factory<type>::get();
	});

	auto coroutine = [result_0 = std::move(result_0)]() mutable->result<type> {
		co_return co_await result_0;
	};

	auto result_1 = coroutine();
	result_1.wait();
	test_ready_result_costume_exception(std::move(result_1), 123456789);
}

void concurrencpp::tests::test_non_ready_result() {
	test_non_ready_result_value_impl<int>();
	test_non_ready_result_value_impl<std::string>();
	test_non_ready_result_value_impl<void>();
	test_non_ready_result_value_impl<int&>();
	test_non_ready_result_value_impl<std::string&>();

	test_non_ready_result_exception_impl<int>();
	test_non_ready_result_exception_impl<std::string>();
	test_non_ready_result_exception_impl<void>();
	test_non_ready_result_exception_impl<int&>();
	test_non_ready_result_exception_impl<std::string&>();
}

template<class type>
result<type> concurrencpp::tests::recursive_coro_value(const size_t cur_depth, const size_t max_depth) {
	if (cur_depth < max_depth) {
		co_return co_await recursive_coro_value<type>(cur_depth + 1, max_depth);
	}

	co_return co_await coroutines::get_coroutine_test_ex()->submit([]() -> decltype(auto) {
		std::this_thread::sleep_for(std::chrono::milliseconds(20));
		return result_factory<type>::get();
	});
}

template<class type>
result<type> concurrencpp::tests::recursive_coro_exception(const size_t cur_depth, const size_t max_depth) {
	if (cur_depth < max_depth) {
		co_return co_await recursive_coro_exception<type>(cur_depth + 1, max_depth);;
	}

	co_return co_await coroutines::get_coroutine_test_ex()->submit([]() -> decltype(auto) {
		std::this_thread::sleep_for(std::chrono::milliseconds(20));
		throw costume_exception(123456789);
		return result_factory<type>::get();
	});
}

template<class type>
void concurrencpp::tests::test_coroutines_recursivly_impl() {
	//value
	{
		auto result = recursive_coro_value<type>(0, 20);
		result.wait();
		test_ready_result_result(std::move(result));
	}

	//exception
	{
		auto result = recursive_coro_exception<type>(0, 20);
		result.wait();
		test_ready_result_costume_exception(std::move(result), 123456789);
	}
}

void concurrencpp::tests::test_coroutines_recursivly() {
	test_coroutines_recursivly_impl<int>();
	test_coroutines_recursivly_impl<std::string>();
	test_coroutines_recursivly_impl<void>();
	test_coroutines_recursivly_impl<int&>();
	test_coroutines_recursivly_impl<std::string&>();
}

void concurrencpp::tests::test_coro_combo_value() {
	std::mutex lock;
	int dummy_integer = 0;
	std::string dummy_string;

	auto int_coro = []() -> result<int> {
		co_return co_await coroutines::get_coroutine_test_ex()->submit([] {
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
			return result_factory<int>::get();
		});
	};

	auto int_result = int_coro();

	auto string_coro = [int_result = std::move(int_result)]() mutable->result<std::string> {
		const auto int_res = co_await int_result;
		co_return std::to_string(int_res);
	};

	auto string_result = string_coro();

	auto int_ref_coro = [&, string_result = std::move(string_result)]() mutable->result<int&> {
		auto str_res = co_await string_result;
		str_res.pop_back();

		{
			std::unique_lock<std::mutex> _lock(lock);
			dummy_integer = std::stoi(str_res);
		}

		co_return dummy_integer;
	};

	auto int_ref_result = int_ref_coro();

	auto str_ref_coro = [&, int_ref_result = std::move(int_ref_result)]() mutable->result<std::string&> {
		auto& int_res = co_await int_ref_result;
		assert_equal(&int_res, &dummy_integer);

		{
			std::unique_lock<std::mutex> _lock(lock);
			dummy_string = std::to_string(int_res);
		}

		co_return dummy_string;
	};

	auto str_ref_result = str_ref_coro();

	auto void_coro = [&, str_ref_result = std::move(str_ref_result)]() mutable->result<void> {
		auto& str_res = co_await str_ref_result;
		assert_equal(&str_res, &dummy_string);
		assert_equal(str_res, "12345678");
	};

	try {
		void_coro().get();
	}
	catch (const std::exception&) {
		assert_true(false);
	}

};

void concurrencpp::tests::test_coro_combo_exception() {
	std::mutex lock;
	int dummy_integer = 0;
	std::string dummy_string;

	auto int_coro = []() ->  result<int> {
		co_return co_await coroutines::get_coroutine_test_ex()->submit([] {
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
			throw costume_exception(123456789);
			return result_factory<int>::get();
		});
	};

	auto int_result = int_coro();

	auto string_coro = [int_result = std::move(int_result)]() mutable->result<std::string> {
		auto int_res = co_await int_result;
		co_return std::to_string(int_res);
	};

	auto string_result = string_coro();

	auto int_ref_coro = [&, string_result = std::move(string_result)]() mutable->result<int&> {
		auto str_res = co_await string_result;
		str_res.pop_back();

		{
			std::unique_lock<std::mutex> _lock(lock);
			dummy_integer = std::stoi(str_res);
		}

		co_return dummy_integer;
	};

	auto int_ref_result = int_ref_coro();

	auto str_ref_coro = [&, int_ref_result = std::move(int_ref_result)]() mutable->result<std::string&> {
		auto& int_res = co_await int_ref_result;
		assert_equal(&int_res, &dummy_integer);

		{
			std::unique_lock<std::mutex> _lock(lock);
			dummy_string = std::to_string(int_res);
		}

		co_return dummy_string;
	};

	auto str_ref_result = str_ref_coro();

	auto void_coro = [&, str_ref_result = std::move(str_ref_result)]() mutable->result<void> {
		auto& str_res = co_await str_ref_result;
		assert_equal(&str_res, &dummy_string);
		assert_equal(str_res, "1234568");
	};

	auto result = void_coro();
	result.wait();
	test_ready_result_costume_exception(std::move(result), 123456789);
}

void concurrencpp::tests::test_coro_combo() {
	test_coro_combo_value();
	test_coro_combo_exception();
}

void concurrencpp::tests::test_co_await() {
	tester tester("coroutines test");

	tester.add_step("coroutines RAII", test_coro_RAII);
	tester.add_step("co_await on ready result", test_ready_result);
	tester.add_step("co_await on unready result", test_non_ready_result);
	tester.add_step("recursive co_await", test_coroutines_recursivly);
	tester.add_step("test compound coroutine", test_coro_combo);

	tester.launch_test();
}