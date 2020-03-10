#ifndef CONCURRENCPP_ASSERTIONS_H
#define CONCURRENCPP_ASSERTIONS_H

#include <string>
#include <cstdio>
#include <thread>

namespace concurrencpp::tests::details{
	std::string to_string(bool value);
	std::string to_string(int value);
	std::string to_string(long value);
	std::string to_string(long long value);
	std::string to_string(unsigned value);
	std::string to_string(unsigned long value);
	std::string to_string(unsigned long long value);
	std::string to_string(float value);
	std::string to_string(double value);
	std::string to_string(long double value);
	const std::string& to_string(const std::string& str);
	std::string to_string(const char* str);
	std::string to_string(const std::string_view str);
	std::string to_string(std::thread::id);
	std::string to_string(std::chrono::time_point<std::chrono::high_resolution_clock> time_point);

	template<class type>
	std::string to_string(type* value) {
		return std::string("pointer[") +
			to_string(reinterpret_cast<intptr_t>(value)) +
			"]";
	}

	template<class type>
	std::string to_string(const type&) {
		return "{object}";
	}

	void assert_same_failed_impl(const std::string& a, const std::string& b);
	void assert_not_same_failed_impl(const std::string& a, const std::string& b);
	void assert_bigger_failed_impl(const std::string& a, const std::string& b);
	void assert_smaller_failed_impl(const std::string& a, const std::string& b);
	void assert_bigger_equal_failed_impl(const std::string& a, const std::string& b);
	void assert_smaller_equal_failed_impl(const std::string& a, const std::string& b);
}

namespace concurrencpp::tests {
	void assert_true(bool condition);
	void assert_false(bool condition);

	template<class a_type, class b_type>
	inline void assert_same(a_type&& a, b_type&& b) {
		if (a == b) {
			return;
		}

		details::assert_same_failed_impl(details::to_string(a), details::to_string(b));
	}

	template<class a_type, class b_type>
	inline void assert_not_same(a_type&& a, b_type&& b) {
		if (a != b) {
			return;
		}

		details::assert_same_failed_impl(details::to_string(a), details::to_string(b));
	}

	template<class a_type, class b_type>
	void assert_bigger(const a_type& a, const b_type& b) {
		if (a > b) {
			return;
		}

		details::assert_bigger_failed_impl(details::to_string(a), details::to_string(b));
	}

	template<class a_type, class b_type>
	void assert_smaller(const a_type& a, const b_type& b) {
		if (a < b) {
			return;
		}

		details::assert_smaller_failed_impl(details::to_string(a), details::to_string(b));
	}

	template<class a_type, class b_type>
	void assert_bigger_equal(const a_type& a, const b_type& b) {
		if (a >= b) {
			return;
		}

		details::assert_bigger_equal_failed_impl(details::to_string(a), details::to_string(b));
	}

	template<class a_type, class b_type>
	void assert_smaller_equal(const a_type& a, const b_type& b) {
		if (a <= b) {
			return;
		}

		details::assert_smaller_equal_failed_impl(details::to_string(a), details::to_string(b));
	}
	template<class exception_type, class task_type>
	void assert_throws(task_type&& task) {
		try {
			task();
		}
		catch (const exception_type&) { return; }
		catch (...) {}

		assert_false(true);
	}
	
}

#endif //CONCURRENCPP_ASSERTIONS_H