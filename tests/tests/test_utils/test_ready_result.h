#ifndef CONCURRENCPP_TEST_READY_RESULT_H
#define CONCURRENCPP_TEST_READY_RESULT_H

#include "concurrencpp.h"

#include "result_factory.h"

#include "../../helpers/assertions.h"

#include <cstdint>

namespace concurrencpp::tests {
	struct costume_exception : public std::exception {
		const intptr_t id;

		costume_exception(intptr_t id) noexcept : id(id) {}
	};
}

namespace concurrencpp::tests {
	template<class type>
	void test_ready_result_result(::concurrencpp::result<type> result, const type& o) {
		assert_true(static_cast<bool>(result));
		assert_equal(result.status(), concurrencpp::result_status::value);

		try
		{
			assert_equal(result.get(), o);
		}
		catch (...) {
			assert_true(false);
		}
	}

	template<class type>
	void test_ready_result_result(
		::concurrencpp::result<type> result,
		std::reference_wrapper<typename std::remove_reference_t<type>> ref) {
		assert_true(static_cast<bool>(result));
		assert_equal(result.status(), concurrencpp::result_status::value);

		try
		{
			assert_equal(&result.get(), &ref.get());
		}
		catch (...) {
			assert_true(false);
		}
	}

	template<class type>
	void test_ready_result_result(::concurrencpp::result<type> result) {
		test_ready_result_result<type>(std::move(result), result_factory<type>::get());
	}

	template<>
	inline void test_ready_result_result(::concurrencpp::result<void> result) {
		assert_true(static_cast<bool>(result));
		assert_equal(result.status(), concurrencpp::result_status::value);
		try
		{
			result.get(); //just make sure no exception is thrown.
		}
		catch (...) {
			assert_true(false);
		}
	}

	template<class type>
	void test_ready_result_costume_exception(concurrencpp::result<type> result, const intptr_t id) {
		assert_true(static_cast<bool>(result));
		assert_equal(result.status(), concurrencpp::result_status::exception);

		try {
			result.get();
		}
		catch (costume_exception e) {
			return assert_equal(e.id, id);
		}
		catch (...) {}

		assert_true(false);
	}
}

#endif