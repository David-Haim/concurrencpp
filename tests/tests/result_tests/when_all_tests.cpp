#include "concurrencpp.h"
#include "../all_tests.h"

#include "result_test_helpers.h"

#include "../executor_tests/executor_test_helpers.h"

#include "../../tester/tester.h"
#include "../../helpers/assertions.h"
#include "../../helpers/random.h"
#include "../../helpers/object_observer.h"

namespace concurrencpp::tests {
	template<class type>
	void test_when_all_vector_empty_result();

	template<class type>
	result<void> test_when_all_vector_impl(std::shared_ptr<thread_executor> ex);

	template<class type>
	void test_when_all_vector_empty_range();

	void test_when_all_vector(std::shared_ptr<thread_executor> ex);

	void test_when_all_tuple_empty_result();

	result<void> test_when_all_tuple_impl(std::shared_ptr<thread_executor> ex);

	void test_when_all_tuple_empty_range();

	void test_when_all_tuple(std::shared_ptr<thread_executor> ex);
}

template <class type>
void concurrencpp::tests::test_when_all_vector_empty_result() {
	const size_t task_count = 63;
	std::vector<result_promise<type>> result_promises(task_count);
	std::vector<result<type>> results;

	for(auto& rp : result_promises) {
		results.emplace_back(rp.get_result());
	}

	results.emplace_back();

	assert_throws_with_error_message<errors::empty_result>(
		[&] {
			concurrencpp::when_all(results.begin(), results.end());
		},
		concurrencpp::details::consts::k_when_all_empty_result_error_msg);

	const auto all_valid = std::all_of(
		results.begin(),
		results.begin() + task_count,
		[](const auto& result) {
			return static_cast<bool>(result);
		});

	assert_true(all_valid);
}

template<class type>
concurrencpp::result<void> concurrencpp::tests::test_when_all_vector_impl(std::shared_ptr<thread_executor> ex) {
	const size_t task_count = 1'024;
	auto values = result_factory<type>::get_many(task_count);
	std::atomic_size_t counter = 0;

	std::vector<result<type>> results;
	results.reserve(1'024);

	for (size_t i = 0; i < task_count; i++) {
		results.emplace_back(ex->submit([&values, &counter, i]() -> type {
			(void)values;
			counter.fetch_add(1, std::memory_order_relaxed);

			if (i % 4 == 0) {
				throw costume_exception(i);
			}

			if constexpr (!std::is_same_v<void, type>) {
				return values[i];
			}
		}));
	}

	auto all = concurrencpp::when_all(results.begin(), results.end());

	const auto all_empty = std::all_of(
		results.begin(),
		results.end(),
		[](const auto& res) {
		return !static_cast<bool>(res);
	});

	assert_true(all_empty);

	auto done_results = co_await all;

	assert_equal(counter.load(std::memory_order_relaxed), results.size());

	const auto all_done = std::all_of(
		done_results.begin(),
		done_results.end(),
		[](const auto& res) {
			return static_cast<bool>(res) && (res.status() != result_status::idle);
		});

	assert_true(all_done);

	for (size_t i = 0; i < task_count; i++) {
		if (i % 4 == 0) {
			test_ready_result_costume_exception(std::move(done_results[i]), i);
		}
		else {
			if constexpr (!std::is_same_v<void, type>) {
				test_ready_result_result(std::move(done_results[i]), values[i]);
			}
			else {
				test_ready_result_result(std::move(done_results[i]));
			}
		}
	}
}

template <class type>
void concurrencpp::tests::test_when_all_vector_empty_range() {
	std::vector<result<type>> empty_range;
	auto all = concurrencpp::when_all(empty_range.begin(), empty_range.end());
	assert_equal(all.status(), result_status::value);
	assert_equal(all.get(), std::vector<result<type>>{});
}

void concurrencpp::tests::test_when_all_vector(std::shared_ptr<thread_executor> ex) {
	test_when_all_vector_empty_result<int>();
	test_when_all_vector_empty_result<std::string>();
	test_when_all_vector_empty_result<void>();
	test_when_all_vector_empty_result<int&>();
	test_when_all_vector_empty_result<std::string&>();

	test_when_all_vector_impl<int>(ex).get();
	test_when_all_vector_impl<std::string>(ex).get();
	test_when_all_vector_impl<void>(ex).get();
	test_when_all_vector_impl<int&>(ex).get();
	test_when_all_vector_impl<std::string&>(ex).get();

	test_when_all_vector_empty_range<int>();
	test_when_all_vector_empty_range<std::string>();
	test_when_all_vector_empty_range<void>();
	test_when_all_vector_empty_range<int&>();
	test_when_all_vector_empty_range<std::string&>();
}

void concurrencpp::tests::test_when_all_tuple_empty_result() {
	result_promise<int> rp_int;
	auto int_res = rp_int.get_result();

	result_promise<std::string> rp_s;
	auto s_res = rp_s.get_result();

	result_promise<void> rp_void;
	auto void_res = rp_void.get_result();

	result_promise<int&> rp_int_ref;
	auto int_ref_res = rp_int_ref.get_result();

	result<std::string&> s_ref_res;

	assert_throws_with_error_message<errors::empty_result>(
		[&] {
			when_all(
				std::move(int_res),
				std::move(s_res),
				std::move(void_res),
				std::move(int_ref_res),
				std::move(s_ref_res));
		},
		concurrencpp::details::consts::k_when_all_empty_result_error_msg);

	assert_true(static_cast<bool>(int_res));
	assert_true(static_cast<bool>(s_res));
	assert_true(static_cast<bool>(void_res));
	assert_true(static_cast<bool>(int_res));
}

concurrencpp::result<void> concurrencpp::tests::test_when_all_tuple_impl(std::shared_ptr<thread_executor> ex) {
	std::atomic_size_t counter = 0;

	auto int_res_val = ex->submit([&]() ->int {
		counter.fetch_add(1, std::memory_order_relaxed);
		return result_factory<int>::get();
	});

	auto int_res_ex = ex->submit([&]() ->int {
		counter.fetch_add(1, std::memory_order_relaxed);
		throw costume_exception(0);
		return result_factory<int>::get();
	});

	auto s_res_val = ex->submit([&]() -> std::string {
		counter.fetch_add(1, std::memory_order_relaxed);
		return result_factory<std::string>::get();
	});

	auto s_res_ex = ex->submit([&]() -> std::string {
		counter.fetch_add(1, std::memory_order_relaxed);
		throw costume_exception(1);
		return result_factory<std::string>::get();
	});

	auto void_res_val = ex->submit([&]{
		counter.fetch_add(1, std::memory_order_relaxed);
	});

	auto void_res_ex = ex->submit([&]{
		counter.fetch_add(1, std::memory_order_relaxed);
		throw costume_exception(2);
	});

	auto int_ref_res_val = ex->submit([&]() ->int& {
		counter.fetch_add(1, std::memory_order_relaxed);
		return result_factory<int&>::get();
	});

	auto int_ref_res_ex = ex->submit([&]() ->int& {
		counter.fetch_add(1, std::memory_order_relaxed);
		throw costume_exception(3);
		return result_factory<int&>::get();
	});

	auto s_ref_res_val = ex->submit([&]() -> std::string& {
		counter.fetch_add(1, std::memory_order_relaxed);
		return result_factory<std::string&>::get();
	});

	auto s_ref_res_ex = ex->submit([&]() -> std::string& {
		counter.fetch_add(1, std::memory_order_relaxed);
		throw costume_exception(4);
		return result_factory<std::string&>::get();
	});

	auto all = when_all(
		std::move(int_res_val),
		std::move(int_res_ex),
		std::move(s_res_val),
		std::move(s_res_ex),
		std::move(void_res_val),
		std::move(void_res_ex),
		std::move(int_ref_res_val),
		std::move(int_ref_res_ex),
		std::move(s_ref_res_val),
		std::move(s_ref_res_ex));

	assert_false(static_cast<bool>(int_res_val));
	assert_false(static_cast<bool>(int_res_ex));
	assert_false(static_cast<bool>(s_res_val));
	assert_false(static_cast<bool>(s_res_ex));
	assert_false(static_cast<bool>(void_res_val));
	assert_false(static_cast<bool>(void_res_ex));
	assert_false(static_cast<bool>(int_ref_res_val));
	assert_false(static_cast<bool>(int_ref_res_ex));
	assert_false(static_cast<bool>(s_ref_res_val));
	assert_false(static_cast<bool>(s_ref_res_ex));

	auto done_results_tuple = co_await all;

	assert_equal(counter.load(std::memory_order_relaxed), size_t(10));

	test_ready_result_result(std::move(std::get<0>(done_results_tuple)), result_factory<int>::get());
	test_ready_result_result(std::move(std::get<2>(done_results_tuple)), result_factory<std::string>::get());
	test_ready_result_result(std::move(std::get<4>(done_results_tuple)));
	test_ready_result_result(std::move(std::get<6>(done_results_tuple)), result_factory<int&>::get());
	test_ready_result_result(std::move(std::get<8>(done_results_tuple)), result_factory<std::string&>::get());

	test_ready_result_costume_exception(std::move(std::get<1>(done_results_tuple)), 0);
	test_ready_result_costume_exception(std::move(std::get<3>(done_results_tuple)), 1);
	test_ready_result_costume_exception(std::move(std::get<5>(done_results_tuple)), 2);
	test_ready_result_costume_exception(std::move(std::get<7>(done_results_tuple)), 3);
	test_ready_result_costume_exception(std::move(std::get<9>(done_results_tuple)), 4);
}

void concurrencpp::tests::test_when_all_tuple_empty_range() {
	auto all = when_all();
	assert_equal(all.status(), result_status::value);
	assert_equal(all.get(), std::tuple<>{});
}

void concurrencpp::tests::test_when_all_tuple(std::shared_ptr<thread_executor> ex) {
	test_when_all_tuple_empty_result();
	test_when_all_tuple_impl(ex);
	test_when_all_tuple_empty_range();
}

void concurrencpp::tests::test_when_all() {
	auto thread_executor = std::make_shared<concurrencpp::thread_executor>();
	executor_shutdowner shutdown(thread_executor);

	tester tester("when_all test");

	tester.add_step("when_all(begin, end)", [thread_executor] {
		test_when_all_vector(thread_executor);
	});

	tester.add_step("when_all(result_types&& ... results)", [thread_executor] {
		test_when_all_tuple(thread_executor);
	});

	tester.launch_test();
}