#ifndef CONCURRENCPP_ASSERTIONS_H
#define CONCURRENCPP_ASSERTIONS_H

#include <cstdint>
#include <string>
#include <string_view>

namespace concurrencpp::tests::details {
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

    template<class type>
    std::string to_string(type* value) {
        return std::string("pointer[") + to_string(reinterpret_cast<intptr_t>(value)) + "]";
    }

    template<class type>
    std::string to_string(const type&) {
        return "{object}";
    }

    void assert_equal_failed_impl(const std::string& a, const std::string& b);
    void assert_not_equal_failed_impl(const std::string& a, const std::string& b);
    void assert_bigger_failed_impl(const std::string& a, const std::string& b);
    void assert_smaller_failed_impl(const std::string& a, const std::string& b);
    void assert_bigger_equal_failed_impl(const std::string& a, const std::string& b);
    void assert_smaller_equal_failed_impl(const std::string& a, const std::string& b);
}  // namespace concurrencpp::tests::details

namespace concurrencpp::tests {
    void assert_true(bool condition);
    void assert_false(bool condition);

    template<class a_type, class b_type>
    void assert_equal(const a_type& given_value, const b_type& expected_value) {
        if (given_value == expected_value) {
            return;
        }

        details::assert_equal_failed_impl(details::to_string(given_value), details::to_string(expected_value));
    }

    template<class a_type, class b_type>
    inline void assert_not_equal(const a_type& given_value, const b_type& expected_value) {
        if (given_value != expected_value) {
            return;
        }

        details::assert_not_equal_failed_impl(details::to_string(given_value), details::to_string(expected_value));
    }

    template<class a_type, class b_type>
    void assert_bigger(const a_type& given_value, const b_type& expected_value) {
        if (given_value > expected_value) {
            return;
        }

        details::assert_bigger_failed_impl(details::to_string(given_value), details::to_string(expected_value));
    }

    template<class a_type, class b_type>
    void assert_smaller(const a_type& given_value, const b_type& expected_value) {
        if (given_value < expected_value) {
            return;
        }

        details::assert_smaller_failed_impl(details::to_string(given_value), details::to_string(expected_value));
    }

    template<class a_type, class b_type>
    void assert_bigger_equal(const a_type& given_value, const b_type& expected_value) {
        if (given_value >= expected_value) {
            return;
        }

        details::assert_bigger_equal_failed_impl(details::to_string(given_value), details::to_string(expected_value));
    }

    template<class a_type, class b_type>
    void assert_smaller_equal(const a_type& given_value, const b_type& expected_value) {
        if (given_value <= expected_value) {
            return;
        }

        details::assert_smaller_equal_failed_impl(details::to_string(given_value), details::to_string(expected_value));
    }

    template<class exception_type, class task_type>
    void assert_throws(task_type&& task) {
        try {
            task();
        } catch (const exception_type&) {
            return;
        } catch (...) {
        }

        assert_false(true);
    }

    template<class exception_type, class task_type>
    void assert_throws_with_error_message(task_type&& task, std::string_view error_msg) {
        try {
            task();
        } catch (const exception_type& e) {
            assert_equal(error_msg, e.what());
            return;
        } catch (...) {
        }

        assert_false(true);
    }

    template<class exception_type, class task_type>
    void assert_throws_contains_error_message(task_type&& task, std::string_view error_msg) {
        try {
            task();
        } catch (const exception_type& e) {
            const auto pos = std::string(e.what()).find(error_msg);
            assert_not_equal(pos, std::string::npos);
            return;
        } catch (...) {
        }

        assert_false(true);
    }

}  // namespace concurrencpp::tests

#endif  // CONCURRENCPP_ASSERTIONS_H
