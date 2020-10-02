#include "concurrencpp.h"
#include "../all_tests.h"

#include "../test_utils/test_ready_result.h"

#include "../../tester/tester.h"
#include "../../helpers/assertions.h"
#include "../../helpers/random.h"
#include "../../helpers/object_observer.h"

namespace concurrencpp::tests {
	template<class type>
	void test_make_ready_result_impl();
	void test_make_ready_result();

	void test_make_exceptional_result();
}

template<class type>
void concurrencpp::tests::test_make_ready_result_impl() {
	result<type> result;

	if constexpr (std::is_same_v<void, type>) {
		result = concurrencpp::make_ready_result<type>();
	}
	else {
		result = make_ready_result<type>(result_factory<type>::get());
	}

	test_ready_result_result(std::move(result));
}

void concurrencpp::tests::test_make_ready_result() {
	test_make_ready_result_impl<int>();
	test_make_ready_result_impl<std::string>();
	test_make_ready_result_impl<void>();
	test_make_ready_result_impl<int&>();
	test_make_ready_result_impl<std::string&>();
}

void concurrencpp::tests::test_make_exceptional_result() {
	assert_throws_with_error_message<std::invalid_argument>([] {
		make_exceptional_result<std::string>({});
	}, concurrencpp::details::consts::k_make_exceptional_result_exception_null_error_msg);

	auto assert_ok = [](result<int>& result) {
		assert_equal(result.status(), result_status::exception);
		try
		{
			result.get();
		}
		catch (const std::runtime_error& re) {
			assert_equal(std::string("error"), re.what());
			return;
		}
		catch (...) {}
		assert_false(true);
	};

	result<int> res = make_exceptional_result<int>(std::runtime_error("error"));
	assert_ok(res);

	res = make_exceptional_result<int>(std::make_exception_ptr(std::runtime_error("error")));
	assert_ok(res);
}

void concurrencpp::tests::test_make_result() {
	tester tester("make_result test");

	tester.add_step("make_ready_result", test_make_ready_result);
	tester.add_step("make_exceptional_result", test_make_exceptional_result);

	tester.launch_test();
}