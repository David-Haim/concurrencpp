#ifndef CONCURRENCPP_EXECUTOR_TEST_HELPERS_H
#define CONCURRENCPP_EXECUTOR_TEST_HELPERS_H

#include "../../helpers/assertions.h"
#include "../../concurrencpp/src/executors/constants.h"
#include "../../concurrencpp/src/executors/executor.h"

namespace concurrencpp::tests {
	class cancellation_tester {

	private:
		std::exception_ptr& m_dest;

	public:
		cancellation_tester(std::exception_ptr& dest) noexcept: m_dest(dest){}
		cancellation_tester(cancellation_tester&&) noexcept = default;

		void operator()(){}

		void cancel(std::exception_ptr reason) noexcept {
			m_dest = reason;
		}
	};

	template<class expected_exception_type>
	void assert_cancelled_correctly(std::exception_ptr exception, std::string_view expected_msg) {
		auto expected_msg_ = expected_msg;
		try {
			std::rethrow_exception(exception);
		}
		catch (expected_exception_type e) {
			assert_same(expected_msg_, e.what());
		}
		catch (...) {
			assert_true(false);
		}
	}
}

#endif