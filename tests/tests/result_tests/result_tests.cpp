#include "concurrencpp.h"
#include "../all_tests.h"

#include "result_test_helpers.h"

#include "../../tester/tester.h"
#include "../../helpers/assertions.h"
#include "../../helpers/random.h"

namespace concurrencpp::tests {
	template<class type>
	void test_result_constructor_impl();
	void test_result_constructor();

	template<class type>
	void test_result_status_impl();
	void test_result_status();

	template<class type>
	void test_result_get_impl();
	void test_result_get();

	template<class type>
	void test_result_wait_impl();
	void test_result_wait();

	template<class type>
	void test_result_wait_for_impl();
	void test_result_wait_for();

	template<class type>
	void test_result_wait_until_impl();
	void test_result_wait_until();

	template<class type>
	void test_result_assignment_operator_impl();
	void test_result_assignment_operator();
}

using concurrencpp::result;
using concurrencpp::result_promise;
using namespace std::chrono;
using namespace concurrencpp::tests;


template<class type>
void concurrencpp::tests::test_result_constructor_impl() {
	result<type> default_constructed_result;
	assert_false(static_cast<bool>(default_constructed_result));

	result_promise<type> tcs;
	auto tcs_result = tcs.get_result();
	assert_true(static_cast<bool>(tcs_result));
	assert_equal(tcs_result.status(), result_status::idle);

	auto new_result = std::move(tcs_result);
	assert_false(static_cast<bool>(tcs_result));
	assert_true(static_cast<bool>(new_result));
	assert_equal(new_result.status(), result_status::idle);
}

void concurrencpp::tests::test_result_constructor() {
	test_result_constructor_impl<int>();
	test_result_constructor_impl<std::string>();
	test_result_constructor_impl<void>();
	test_result_constructor_impl<int&>();
	test_result_constructor_impl<std::string&>();
}

template<class type>
void concurrencpp::tests::test_result_status_impl() {
	//empty result throws
	{
		result<type> result;
		assert_throws<concurrencpp::errors::empty_result>([&result] { result.status(); });
	}

	//idle result
	{
		result_promise<type> tcs;
		auto result = tcs.get_result();
		assert_equal(result.status(), result_status::idle);
	}

	//ready by value
	{
		result_promise<type> tcs;
		auto result = tcs.get_result();
		tcs.set_from_function(result_factory<type>::get);
		assert_equal(result.status(), result_status::value);
	}

	//exception result
	{
		result_promise<type> tcs;
		auto result = tcs.get_result();
		tcs.set_from_function(result_factory<type>::throw_ex);
		assert_equal(result.status(), result_status::exception);
	}

	//multiple calls of status are ok
	{
		result_promise<type> tcs;
		auto result = tcs.get_result();
		tcs.set_from_function(result_factory<type>::get);

		for (size_t i = 0; i < 10; i++) {
			assert_equal(result.status(), result_status::value);
		}
	}
}

void concurrencpp::tests::test_result_status() {
	test_result_status_impl<int>();
	test_result_status_impl<std::string>();
	test_result_status_impl<void>();
	test_result_status_impl<int&>();
	test_result_status_impl<std::string&>();
}

template<class type>
void concurrencpp::tests::test_result_get_impl() {
	//empty result throws
	{
		result<type> result;
		assert_throws<concurrencpp::errors::empty_result>([&result] { result.get(); });
	}

	//get blocks until value is present and empties the result
	{
		result_promise<type> tcs;
		auto result = tcs.get_result();
		const auto unblocking_time = high_resolution_clock::now() + seconds(2);

		std::thread thread([tcs = std::move(tcs), unblocking_time]() mutable {
			std::this_thread::sleep_until(unblocking_time);
			tcs.set_from_function(result_factory<type>::get);
		});

		result.get();
		const auto now = high_resolution_clock::now();

		assert_false(static_cast<bool>(result));
		assert_bigger_equal(now, unblocking_time);
		assert_smaller(now, unblocking_time + seconds(2));
		thread.join();
	}

	//get blocks until exception is present and empties the result
	{
		random randomizer;
		result_promise<type> tcs;
		auto result = tcs.get_result();
		const auto id = randomizer();
		const auto unblocking_time = high_resolution_clock::now() + seconds(2);

		std::thread thread([tcs = std::move(tcs), id, unblocking_time]() mutable {
			std::this_thread::sleep_until(unblocking_time);
			tcs.set_exception(std::make_exception_ptr(costume_exception(id)));
		});

		try {
			result.get();
		}
		catch (costume_exception e) {
			assert_equal(e.id, id);
		}

		const auto now = high_resolution_clock::now();

		assert_false(static_cast<bool>(result));
		assert_bigger_equal(now, unblocking_time);
		assert_smaller(now, unblocking_time + seconds(2));
		thread.join();
	}
}

void concurrencpp::tests::test_result_get() {
	test_result_get_impl<int>();
	test_result_get_impl<std::string>();
	test_result_get_impl<void>();
	test_result_get_impl<int&>();
	test_result_get_impl<std::string&>();
}

template<class type>
void concurrencpp::tests::test_result_wait_impl() {
	//empty result throws
	{
		result<type> result;
		assert_throws<concurrencpp::errors::empty_result>([&result] { result.wait(); });
	}

	//wait blocks until value is present
	{
		result_promise<type> tcs;
		auto result = tcs.get_result();
		const auto unblocking_time = high_resolution_clock::now() + seconds(2);

		std::thread thread([tcs = std::move(tcs), unblocking_time]() mutable {
			std::this_thread::sleep_until(unblocking_time);
			tcs.set_from_function(result_factory<type>::get);
		});

		result.wait();
		const auto now = high_resolution_clock::now();

		assert_bigger_equal(now, unblocking_time);
		assert_smaller(now, unblocking_time + seconds(2));

		test_ready_result_result(std::move(result));
		thread.join();
	}

	//wait blocks until exception is present
	{
		random randomizer;
		result_promise<type> tcs;
		auto result = tcs.get_result();
		const auto id = randomizer();
		const auto unblocking_time = high_resolution_clock::now() + seconds(2);

		std::thread thread([tcs = std::move(tcs), id, unblocking_time]() mutable {
			std::this_thread::sleep_until(unblocking_time);
			tcs.set_exception(std::make_exception_ptr(costume_exception(id)));
		});

		result.wait();
		const auto now = high_resolution_clock::now();

		assert_bigger_equal(now, unblocking_time);
		assert_smaller(now, unblocking_time + seconds(2));

		test_ready_result_costume_exception(std::move(result), id);
		thread.join();
	}

	//if result is ready with value, wait returns immediately
	{
		result_promise<type> tcs;
		auto result = tcs.get_result();
		tcs.set_from_function(result_factory<type>::get);

		const auto time_before = high_resolution_clock::now();
		result.wait();
		const auto time_after = high_resolution_clock::now();
		const auto total_blocking_time = duration_cast<milliseconds>(time_after - time_before).count();

		assert_smaller_equal(total_blocking_time, 3);
		test_ready_result_result(std::move(result));
	}

	//if result is ready with exception, wait returns immediately
	{
		random randomizer;
		result_promise<type> tcs;
		auto result = tcs.get_result();
		const auto id = randomizer();

		tcs.set_exception(std::make_exception_ptr(costume_exception(id)));

		const auto time_before = high_resolution_clock::now();
		result.wait();
		const auto time_after = high_resolution_clock::now();
		const auto total_blocking_time = duration_cast<milliseconds>(time_after - time_before).count();

		assert_smaller_equal(total_blocking_time, 3);
		test_ready_result_costume_exception(std::move(result), id);
	}

	//multiple calls to wait are ok
	{
		result_promise<type> tcs;
		auto result = tcs.get_result();
		const auto unblocking_time = high_resolution_clock::now() + seconds(1);

		std::thread thread([tcs = std::move(tcs), unblocking_time]() mutable {
			std::this_thread::sleep_until(unblocking_time);
			tcs.set_from_function(result_factory<type>::get);
		});

		for (size_t i = 0; i < 10; i++) {
			result.wait();
		}

		test_ready_result_result(std::move(result));
		thread.join();
	}
}

void concurrencpp::tests::test_result_wait() {
	test_result_wait_impl<int>();
	test_result_wait_impl<std::string>();
	test_result_wait_impl<void>();
	test_result_wait_impl<int&>();
	test_result_wait_impl<std::string&>();
}

template<class type>
void concurrencpp::tests::test_result_wait_for_impl() {
	//empty result throws
	{
		concurrencpp::result<type> result;
		assert_throws<concurrencpp::errors::empty_result>([&result] {
			result.wait_for(seconds(1));
		});
	}

	//if the result is ready by value, don't block and return status::value
	{
		result_promise<type> tcs;
		auto result = tcs.get_result();

		tcs.set_from_function(result_factory<type>::get);

		const auto before = high_resolution_clock::now();
		const auto status = result.wait_for(seconds(3));
		const auto after = high_resolution_clock::now();
		const auto time = duration_cast<milliseconds>(after - before).count();

		assert_smaller_equal(time, 3);
		assert_equal(status, result_status::value);
	}

	//if the result is ready by exception, don't block and return status::exception
	{
		result_promise<type> tcs;
		auto result = tcs.get_result();

		tcs.set_from_function(result_factory<type>::throw_ex);

		const auto before = high_resolution_clock::now();
		const auto status = result.wait_for(seconds(3));
		const auto after = high_resolution_clock::now();
		const auto time = duration_cast<milliseconds>(after - before).count();

		assert_smaller_equal(time, 3);
		assert_equal(status, result_status::exception);
	}

	//if timeout reaches and no value/exception - return status::idle
	{
		result_promise<type> tcs;
		auto result = tcs.get_result();

		const auto before = high_resolution_clock::now();
		const auto status = result.wait_for(seconds(2));
		const auto after = high_resolution_clock::now();
		const auto time = duration_cast<seconds>(after - before);

		assert_equal(status, result_status::idle);
		assert_bigger_equal(time.count(), 2);
	}

	//if result is set before timeout, unblock, and return status::value
	{
		result_promise<type> tcs;
		auto result = tcs.get_result();
		const auto unblocking_time = high_resolution_clock::now() + seconds(2);

		std::thread thread([tcs = std::move(tcs), unblocking_time]() mutable{
			std::this_thread::sleep_until(unblocking_time);
			tcs.set_from_function(result_factory<type>::get);
		});

		result.wait_for(seconds(100));
		const auto now = high_resolution_clock::now();

		test_ready_result_result(std::move(result));
		assert_bigger_equal(now, unblocking_time);
		assert_smaller(now, unblocking_time + seconds(2));
		thread.join();
	}

	//if exception is set before timeout, unblock, and return status::exception
	{
		random randomizer;
		result_promise<type> tcs;
		auto result = tcs.get_result();
		const auto id = randomizer();
		const auto unblocking_time = high_resolution_clock::now() + seconds(2);

		std::thread thread([tcs = std::move(tcs), unblocking_time, id]() mutable{
			std::this_thread::sleep_until(unblocking_time);
			tcs.set_exception(std::make_exception_ptr(costume_exception(id)));
		});

		result.wait_for(seconds(100));
		const auto now = high_resolution_clock::now();

		test_ready_result_costume_exception(std::move(result), id);
		assert_bigger_equal(now, unblocking_time);
		assert_smaller(now, unblocking_time + seconds(2));

		thread.join();
	}

	//multiple calls of wait_for are ok
	{
		result_promise<type> tcs;
		auto result = tcs.get_result();
		const auto unblocking_time = high_resolution_clock::now() + seconds(2);

		std::thread thread([tcs = std::move(tcs), unblocking_time]() mutable{
			std::this_thread::sleep_until(unblocking_time);
			tcs.set_from_function(result_factory<type>::get);
		});

		for (size_t i = 0; i < 10; i++) {
			result.wait_for(seconds(10));
		}

		test_ready_result_result(std::move(result));
		thread.join();
	}
}

void concurrencpp::tests::test_result_wait_for() {
	test_result_wait_for_impl<int>();
	test_result_wait_for_impl<std::string>();
	test_result_wait_for_impl<void>();
	test_result_wait_for_impl<int&>();
	test_result_wait_for_impl<std::string&>();
}

template<class type>
void concurrencpp::tests::test_result_wait_until_impl() {
	//empty result throws
	{
		concurrencpp::result<type> result;
		assert_throws<concurrencpp::errors::empty_result>([&result] {
			const auto later = high_resolution_clock::now() + seconds(10);
			result.wait_until(later);
		});
	}

	//if time_point <= now, the function is equivilent to result::status
	{
		result_promise<type> tcs_idle, tcs_val, tcs_err;
		result<type> idle_result = tcs_idle.get_result(),
			value_result = tcs_val.get_result(),
			err_result = tcs_err.get_result();

		tcs_val.set_from_function(result_factory<type>::get);
		tcs_err.set_from_function(result_factory<type>::throw_ex);

		const auto now = high_resolution_clock::now();
		std::this_thread::sleep_for(seconds(2));

		assert_equal(idle_result.wait_until(now), concurrencpp::result_status::idle);
		assert_equal(value_result.wait_until(now), concurrencpp::result_status::value);
		assert_equal(err_result.wait_until(now), concurrencpp::result_status::exception);
	}

	//if the result is ready by value, don't block and return status::value
	{
		result_promise<type> tcs;
		auto result = tcs.get_result();

		tcs.set_from_function(result_factory<type>::get);

		const auto later = high_resolution_clock::now() + seconds(2);
		const auto before = high_resolution_clock::now();
		const auto status = result.wait_until(later);
		const auto after = high_resolution_clock::now();
		const auto time = duration_cast<milliseconds>(after - before).count();

		assert_smaller_equal(time, 3);
		assert_equal(status, result_status::value);
	}

	//if the result is ready by exception, don't block and return status::exception
	{
		result_promise<type> tcs;
		auto result = tcs.get_result();

		tcs.set_from_function(result_factory<type>::throw_ex);

		const auto later = high_resolution_clock::now() + seconds(2);
		const auto before = high_resolution_clock::now();
		const auto status = result.wait_until(later);
		const auto after = high_resolution_clock::now();
		const auto time = duration_cast<milliseconds>(after - before).count();

		assert_smaller_equal(time, 3);
		assert_equal(status, result_status::exception);
	}

	//if timeout reaches and no value/exception - return status::idle
	{
		result_promise<type> tcs;
		auto result = tcs.get_result();

		const auto later = high_resolution_clock::now() + seconds(2);
		const auto status = result.wait_until(later);
		const auto now = high_resolution_clock::now();

		assert_equal(status, result_status::idle);
		assert_bigger_equal(now, later);
	}

	//if result is set before timeout, unblock, and return status::value
	{
		result_promise<type> tcs;
		auto result = tcs.get_result();
		const auto unblocking_time = high_resolution_clock::now() + seconds(2);
		const auto later = high_resolution_clock::now() + seconds(10);

		std::thread thread([tcs = std::move(tcs), unblocking_time]() mutable{
			std::this_thread::sleep_until(unblocking_time);
			tcs.set_from_function(result_factory<type>::get);
		});

		result.wait_until(later);
		const auto now = high_resolution_clock::now();

		test_ready_result_result(std::move(result));
		assert_bigger_equal(now, unblocking_time);
		assert_smaller(now, unblocking_time + seconds(2));
		thread.join();
	}

	//if exception is set before timeout, unblock, and return status::exception
	{
		random randomizer;
		result_promise<type> tcs;
		auto result = tcs.get_result();
		const auto id = randomizer();

		const auto unblocking_time = high_resolution_clock::now() + seconds(2);
		const auto later = high_resolution_clock::now() + seconds(10);

		std::thread thread([tcs = std::move(tcs), unblocking_time, id]() mutable{
			std::this_thread::sleep_until(unblocking_time);
			tcs.set_exception(std::make_exception_ptr(costume_exception(id)));
		});

		result.wait_until(later);
		const auto now = high_resolution_clock::now();

		test_ready_result_costume_exception(std::move(result), id);
		assert_bigger_equal(now, unblocking_time);
		assert_smaller_equal(now, unblocking_time + seconds(2));
		thread.join();
	}

	//multiple calls to wait_until are ok
	{
		result_promise<type> tcs;
		auto result = tcs.get_result();
		const auto unblocking_time = high_resolution_clock::now() + seconds(2);

		std::thread thread([tcs = std::move(tcs), unblocking_time]() mutable{
			std::this_thread::sleep_until(unblocking_time);
			tcs.set_from_function(result_factory<type>::get);
		});

		for (size_t i = 0; i < 10; i++) {
			const auto later = high_resolution_clock::now() + seconds(1);
			result.wait_until(later);
		}

		thread.join();
	}
}

void concurrencpp::tests::test_result_wait_until() {
	test_result_wait_until_impl<int>();
	test_result_wait_until_impl<std::string>();
	test_result_wait_until_impl<void>();
	test_result_wait_until_impl<int&>();
	test_result_wait_until_impl<std::string&>();
}

template<class type>
void concurrencpp::tests::test_result_assignment_operator_impl() {
	//empty result -> empty result
	{
		result<type> result_0, result_1;
		result_0 = std::move(result_1);
		assert_false(static_cast<bool>(result_0));
		assert_false(static_cast<bool>(result_1));
	}

	//non empty result -> empty result
	{
		result_promise<type> tcs_1;
		result<type> result_0, result_1 = tcs_1.get_result();
		result_0 = std::move(result_1);
		assert_true(static_cast<bool>(result_0));
		assert_false(static_cast<bool>(result_1));

		tcs_1.set_from_function(result_factory<type>::get);
		test_ready_result_result(std::move(result_0));
	}

	//empty result -> non empty result
	{
		result_promise<type> tcs_0;
		result<type> result_0 = tcs_0.get_result(), result_1;
		result_0 = std::move(result_1);
		assert_false(static_cast<bool>(result_0));
		assert_false(static_cast<bool>(result_1));
	}

	//non empty result -> non empty result
	{
		result_promise<type> tcs_0, tcs_1;
		result<type> result_0 = tcs_0.get_result(), result_1 = tcs_1.get_result();
		result_0 = std::move(result_1);
		assert_true(static_cast<bool>(result_0));
		assert_false(static_cast<bool>(result_1));

		tcs_0.set_from_function(result_factory<type>::get);
		assert_false(static_cast<bool>(result_1));
		assert_equal(result_0.status(), result_status::idle);

		tcs_1.set_from_function(result_factory<type>::get);
		assert_false(static_cast<bool>(result_1));
		test_ready_result_result(std::move(result_0));
	}
}

void concurrencpp::tests::test_result_assignment_operator() {
	test_result_assignment_operator_impl<int>();
	test_result_assignment_operator_impl<std::string>();
	test_result_assignment_operator_impl<void>();
	test_result_assignment_operator_impl<int&>();
	test_result_assignment_operator_impl<std::string&>();
}

void concurrencpp::tests::test_result() {
	tester tester("result test");

	tester.add_step("constructor", test_result_constructor);
	tester.add_step("status", test_result_status);
	tester.add_step("get", test_result_get);
	tester.add_step("wait", test_result_wait);
	tester.add_step("wait_for", test_result_wait_for);
	tester.add_step("wait_until", test_result_wait_until);
	tester.add_step("operator =", test_result_assignment_operator);

	tester.launch_test();
}