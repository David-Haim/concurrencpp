#include "concurrencpp.h"
#include "../all_tests.h"

#include "result_test_helpers.h"

#include "../../tester/tester.h"
#include "../../helpers/assertions.h"
#include "../../helpers/random.h"

#include <experimental/coroutine>

namespace concurrencpp::tests {
	template<class type>
	result<void> test_result_resolve_ready_val();

	template<class type>
	result<void> test_result_resolve_ready_err();

	template<class type>
	result<void> test_result_resolve_not_ready_val();

	template<class type>
	result<void> test_result_resolve_not_ready_err();

	template<class type>
	void test_result_resolve_impl();
	void test_result_resolve();

	template<class type>
	result<void> test_result_resolve_via_ready_val();

	template<class type>
	result<void> test_result_resolve_via_ready_err();

	template<class type>
	result<void> test_result_resolve_via_ready_val_force_rescheduling();

	template<class type>
	result<void> test_result_resolve_via_ready_err_force_rescheduling();

	template<class type>
	result<void> test_result_resolve_via_ready_val_force_rescheduling_executor_threw();

	template<class type>
	result<void> test_result_resolve_via_ready_err_force_rescheduling_executor_threw();

	template<class type>
	result<void> test_result_resolve_via_not_ready_val();
	template<class type>
	result<void> test_result_resolve_via_not_ready_err();

	template<class type>
	result<void> test_result_resolve_via_not_ready_val_executor_threw();
	template<class type>
	result<void> test_result_resolve_via_not_ready_err_executor_threw();

	template<class type>
	void test_result_resolve_via_impl();
	void test_result_resolve_via();

}

using concurrencpp::result;
using concurrencpp::result_promise;
using namespace std::chrono;

template<class type>
result<void> concurrencpp::tests::test_result_resolve_ready_val() {
	result_promise<type> rp;
	auto result = rp.get_result();

	rp.set_from_function(result_factory<type>::get);

	const auto thread_id_0 = std::this_thread::get_id();
	const auto time_before = high_resolution_clock::now();

	auto done_result = co_await result.resolve();

	const auto time_after = high_resolution_clock::now();
	const auto thread_id_1 = std::this_thread::get_id();

	const auto elapsed_time = duration_cast<milliseconds>(time_after - time_before).count();

	assert_false(static_cast<bool>(result));
	assert_equal(thread_id_0, thread_id_1);
	assert_smaller_equal(elapsed_time, 10);
	test_ready_result_result(std::move(done_result));
}

template<class type>
result<void> concurrencpp::tests::test_result_resolve_ready_err() {
	random randomizer;
	result_promise<type> rp;
	const auto id = randomizer();
	auto result = rp.get_result();

	rp.set_exception(std::make_exception_ptr(costume_exception(id)));

	const auto thread_id_0 = std::this_thread::get_id();
	const auto time_before = high_resolution_clock::now();

	auto done_result = co_await result.resolve();

	const auto time_after = high_resolution_clock::now();
	const auto thread_id_1 = std::this_thread::get_id();

	auto elapsed_time = duration_cast<milliseconds>(time_after - time_before).count();

	assert_false(static_cast<bool>(result));
	assert_equal(thread_id_0, thread_id_1);
	assert_smaller_equal(elapsed_time, 10);
	test_ready_result_costume_exception(std::move(done_result), id);
}

template<class type>
result<void> concurrencpp::tests::test_result_resolve_not_ready_val() {
	result_promise<type> rp;
	auto result = rp.get_result();
	auto executor = make_test_executor();

	executor->set_rp_value(std::move(rp));

	auto done_result = co_await result.resolve();

	assert_false(static_cast<bool>(result));
	assert_true(executor->scheduled_inline()); //the thread that set the value is the thread that resumes the coroutine.
	test_ready_result_result(std::move(done_result));
}

template<class type>
result<void> concurrencpp::tests::test_result_resolve_not_ready_err() {
	result_promise<type> rp;
	auto result = rp.get_result();
	auto executor = make_test_executor();

	const auto id = executor->set_rp_err(std::move(rp));

	auto done_result = co_await result.resolve();

	assert_false(static_cast<bool>(result));
	assert_true(executor->scheduled_inline()); //the thread that set the err is the thread that resumes the coroutine.
	test_ready_result_costume_exception(std::move(done_result), id);
}

template<class type>
void concurrencpp::tests::test_result_resolve_impl() {
	//empty result throws
	{
		assert_throws<concurrencpp::errors::empty_result>([] {
			result<type> result;
			result.resolve();
		});
	}

	//ready result resumes immediately in the awaiting thread with no rescheduling
	test_result_resolve_ready_val<type>().get();

	//ready result resumes immediately in the awaiting thread with no rescheduling
	test_result_resolve_ready_err<type>().get();

	//if value or exception are not available - suspend and resume in the setting thread
	test_result_resolve_not_ready_val<type>().get();

	//if value or exception are not available - suspend and resume in the setting thread
	test_result_resolve_not_ready_err<type>().get();
}

void concurrencpp::tests::test_result_resolve() {
	test_result_resolve_impl<int>();
	test_result_resolve_impl<std::string>();
	test_result_resolve_impl<void>();
	test_result_resolve_impl<int&>();
	test_result_resolve_impl<std::string&>();
}

template<class type>
result<void> concurrencpp::tests::test_result_resolve_via_ready_val() {
	auto executor = make_test_executor();
	result_promise<type> rp;
	auto result = rp.get_result();

	rp.set_from_function(result_factory<type>::get);

	const auto thread_id_0 = std::this_thread::get_id();
	const auto time_before = high_resolution_clock::now();

	auto done_result = co_await result.resolve_via(executor, false);

	const auto time_after = high_resolution_clock::now();
	const auto thread_id_1 = std::this_thread::get_id();

	const auto elapsed_time = duration_cast<milliseconds>(time_after - time_before).count();

	assert_false(static_cast<bool>(result));
	assert_equal(thread_id_0, thread_id_1);
	assert_smaller_equal(elapsed_time, 10);
	assert_false(executor->scheduled_async());
	test_ready_result_result(std::move(done_result));
}

template<class type>
result<void> concurrencpp::tests::test_result_resolve_via_ready_err() {
	random randomizer;
	result_promise<type> rp;
	const auto id = randomizer();
	auto result = rp.get_result();
	auto executor = make_test_executor();

	rp.set_exception(std::make_exception_ptr(costume_exception(id)));

	const auto thread_id_0 = std::this_thread::get_id();
	const auto time_before = high_resolution_clock::now();

	auto done_result = co_await result.resolve_via(executor, false);

	const auto time_after = high_resolution_clock::now();
	const auto thread_id_1 = std::this_thread::get_id();

	const auto elapsed_time = duration_cast<milliseconds>(time_after - time_before).count();

	assert_false(static_cast<bool>(result));
	assert_equal(thread_id_0, thread_id_1);
	assert_smaller_equal(elapsed_time, 10);
	assert_false(executor->scheduled_async());
	test_ready_result_costume_exception(std::move(done_result), id);
}

template<class type>
result<void> concurrencpp::tests::test_result_resolve_via_ready_val_force_rescheduling() {
	result_promise<type> rp;
	auto result = rp.get_result();
	auto executor = make_test_executor();

	rp.set_from_function(result_factory<type>::get);

	auto done_result = co_await result.resolve_via(executor, true);

	assert_false(static_cast<bool>(result));
	assert_false(executor->scheduled_inline());
	assert_true(executor->scheduled_async());
	test_ready_result_result(std::move(done_result));
}

template<class type>
result<void> concurrencpp::tests::test_result_resolve_via_ready_err_force_rescheduling() {
	random randomizer;
	result_promise<type> rp;
	auto id = static_cast<size_t>(randomizer());
	auto executor = make_test_executor();
	auto result = rp.get_result();

	rp.set_exception(std::make_exception_ptr(costume_exception(id)));

	auto done_result = co_await result.resolve_via(executor, true);

	assert_false(static_cast<bool>(result));
	assert_false(executor->scheduled_inline());
	assert_true(executor->scheduled_async());
	test_ready_result_costume_exception(std::move(done_result), id);
}

template<class type>
result<void> concurrencpp::tests::test_result_resolve_via_ready_val_force_rescheduling_executor_threw() {
	result_promise<type> rp;
	auto result = rp.get_result();
	auto te = std::make_shared<throwing_executor>();

	rp.set_from_function(result_factory<type>::get);

	const auto thread_id_0 = std::this_thread::get_id();

	try {
		auto done_result = co_await result.resolve_via(te, true);
	}
	catch (std::runtime_error re) {
		//do nothing
	}
	catch (...)
	{
		assert_false(true);
	}

	const auto thread_id_1 = std::this_thread::get_id();

	assert_equal(thread_id_0, thread_id_1);
	assert_false(static_cast<bool>(result));
}

template<class type>
result<void> concurrencpp::tests::test_result_resolve_via_ready_err_force_rescheduling_executor_threw() {
	result_promise<type> rp;
	auto result = rp.get_result();
	auto te = std::make_shared<throwing_executor>();

	rp.set_from_function(result_factory<type>::throw_ex);

	const auto thread_id_0 = std::this_thread::get_id();

	try {
		auto done_result = co_await result.resolve_via(te, true);
	}
	catch (std::runtime_error re) {
		//do nothing
	}
	catch (...)
	{
		assert_false(true);
	}

	const auto thread_id_1 = std::this_thread::get_id();

	assert_equal(thread_id_0, thread_id_1);
	assert_false(static_cast<bool>(result));
}

template<class type>
result<void> concurrencpp::tests::test_result_resolve_via_not_ready_val() {
	result_promise<type> rp;
	auto result = rp.get_result();
	auto executor = make_test_executor();

	executor->set_rp_value(std::move(rp));

	auto done_result = co_await result.resolve_via(executor, false);

	assert_false(static_cast<bool>(result));
	assert_false(executor->scheduled_inline());
	assert_true(executor->scheduled_async());
	test_ready_result_result(std::move(done_result));
}

template<class type>
result<void> concurrencpp::tests::test_result_resolve_via_not_ready_err() {
	result_promise<type> rp;
	auto result = rp.get_result();
	auto executor = make_test_executor();

	const auto id = executor->set_rp_err(std::move(rp));

	auto done_result = co_await result.resolve_via(executor, false);

	assert_false(static_cast<bool>(result));
	assert_true(executor->scheduled_async());
	assert_false(executor->scheduled_inline());
	test_ready_result_costume_exception(std::move(done_result), id);
}

template<class type>
result<void> concurrencpp::tests::test_result_resolve_via_not_ready_val_executor_threw() {
	result_promise<type> rp;
	auto result = rp.get_result();

	auto ex = make_test_executor();
	auto te = std::make_shared<throwing_executor>();

	ex->set_rp_value(std::move(rp));

	auto done_result = co_await result.resolve_via(te, false);

	assert_false(static_cast<bool>(result));
	assert_true(ex->scheduled_inline()); //since te threw, execution is resumed in ex::m_setting_thread
	test_executor_error_thrown(std::move(done_result), te);
}

template<class type>
result<void> concurrencpp::tests::test_result_resolve_via_not_ready_err_executor_threw() {
	result_promise<type> rp;
	auto result = rp.get_result();

	auto ex = make_test_executor();
	auto te = std::make_shared<throwing_executor>();

	const auto id = ex->set_rp_err(std::move(rp));
	(void)id;

	auto done_result = co_await result.resolve_via(te, false);

	assert_false(static_cast<bool>(result));
	assert_true(ex->scheduled_inline()); //since te threw, execution is resumed in ex::m_setting_thread
	test_executor_error_thrown(std::move(done_result), te);
}

template<class type>
void concurrencpp::tests::test_result_resolve_via_impl() {
	//empty result throws
	assert_throws<concurrencpp::errors::empty_result>([] {
		result<type> result;
		auto executor = make_test_executor();
		result.resolve_via(executor);
	});

	//if the result is ready by value, and force_rescheduling = false, resume in the calling thread
	test_result_resolve_via_ready_val<type>().get();

	//if the result is ready by exception, and force_rescheduling = false, resume in the calling thread
	test_result_resolve_via_ready_err<type>().get();

	//if the result is ready by value, and force_rescheduling = true, forcfully resume execution through the executor
	test_result_resolve_via_ready_val_force_rescheduling<type>().get();

	//if the result is ready by exception, and force_rescheduling = true, forcfully resume execution through the executor
	test_result_resolve_via_ready_err_force_rescheduling<type>().get();

	//if execution is rescheduled by a throwing executor, reschdule inline and throw executor_exception
	test_result_resolve_via_ready_val_force_rescheduling_executor_threw<type>().get();

	//if execution is rescheduled by a throwing executor, reschdule inline and throw executor_exception
	test_result_resolve_via_ready_err_force_rescheduling_executor_threw<type>().get();

	//if result is not ready - the execution resumes through the executor
	test_result_resolve_not_ready_val<type>().get();

	//if result is not ready - the execution resumes through the executor
	test_result_resolve_not_ready_err<type>().get();

	//if result is not ready - the execution resumes through the executor
	test_result_resolve_via_not_ready_val_executor_threw<type>().get();

	//if result is not ready - the execution resumes through the executor
	test_result_resolve_via_not_ready_err_executor_threw<type>().get();
}

void concurrencpp::tests::test_result_resolve_via() {
	test_result_resolve_via_impl<int>();
	test_result_resolve_via_impl<std::string>();
	test_result_resolve_via_impl<void>();
	test_result_resolve_via_impl<int&>();
	test_result_resolve_via_impl<std::string&>();
}

void concurrencpp::tests::test_result_resolve_all() {
	tester tester("result::resolve, result::resolve_via test");

	tester.add_step("reslove", test_result_resolve);
	tester.add_step("reslove_via", test_result_resolve_via);

	tester.launch_test();
}