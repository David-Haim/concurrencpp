#ifndef CONCURRENCPP_ASSERTIONS_H
#define CONCURRENCPP_ASSERTIONS_H

#include <cstdint>
#include <string>
#include <chrono>
#include <thread>
#include <string_view>
#include <source_location>

#include "concurrencpp/results/generator.h"
#include "concurrencpp/results/result_fwd_declarations.h"

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
    const char* to_string(result_status status);
    std::string to_string(std::chrono::milliseconds ms);
    std::string to_string(std::chrono::steady_clock::time_point time_point);
    std::string to_string(std::chrono::system_clock::time_point time_point);
    std::string to_string(std::tuple<> tp);
    std::string to_string(std::thread::id id);

    template<class type>
    std::string to_string(const type* value) {
        return std::string("T* (") + to_string(reinterpret_cast<intptr_t>(value)) + ")";
    }

    template<class type>
    std::string to_string(const std::shared_ptr<type>& ptr) {
        return std::string("std::shared_ptr<T> (") + to_string(reinterpret_cast<intptr_t>(ptr.get())) + ")";
    }

    template<class type>
    std::string to_string(const concurrencpp::details::generator_iterator<type>& it) {
        return std::string("concurrencpp::generator_iterator<T> (") +
            std::to_string(reinterpret_cast<std::uintptr_t>(get_raw_iterator_ptr(it))) + ")";
    }

    inline const char* to_string(concurrencpp::details::generator_end_iterator) {
        return "concurrencpp::generator_end_iterator";
    }

    void assert_equal_failed_impl(const std::string& a, const std::string& b, const std::source_location& location);
    void assert_not_equal_failed_impl(const std::string& a, const std::string& b, const std::source_location& location);
    void assert_bigger_failed_impl(const std::string& a, const std::string& b, const std::source_location& location);
    void assert_smaller_failed_impl(const std::string& a, const std::string& b, const std::source_location& location);
    void assert_bigger_equal_failed_impl(const std::string& a, const std::string& b, const std::source_location& location);
    void assert_smaller_equal_failed_impl(const std::string& a, const std::string& b, const std::source_location& location);

    void add_source_info_msg(std::stringstream& stream, const std::source_location& location);
    void log_error_and_abort(std::stringstream& stream);
}  // namespace concurrencpp::tests::details

namespace concurrencpp::tests {
    void assert_true(bool condition, const std::source_location location = std::source_location::current());
    void assert_false(bool condition, const std::source_location location = std::source_location::current());

    template<class a_type, class b_type>
    void assert_equal(const a_type& given_value,
                      const b_type& expected_value,
                      const std::source_location location = std::source_location::current()) {
        if (given_value == expected_value) {
            return;
        }

        details::assert_equal_failed_impl(details::to_string(given_value), details::to_string(expected_value), location);
    }

    template<class a_type, class b_type>
    inline void assert_not_equal(const a_type& given_value,
                                 const b_type& expected_value,
                                 const std::source_location location = std::source_location::current()) {
        if (given_value != expected_value) {
            return;
        }

        details::assert_not_equal_failed_impl(details::to_string(given_value), details::to_string(expected_value), location);
    }

    template<class a_type, class b_type>
    void assert_bigger(const a_type& given_value,
                       const b_type& expected_value,
                       const std::source_location location = std::source_location::current()) {
        if (given_value > expected_value) {
            return;
        }

        details::assert_bigger_failed_impl(details::to_string(given_value), details::to_string(expected_value), location);
    }

    template<class a_type, class b_type>
    void assert_smaller(const a_type& given_value,
                        const b_type& expected_value,
                        const std::source_location location = std::source_location::current()) {
        if (given_value < expected_value) {
            return;
        }

        details::assert_smaller_failed_impl(details::to_string(given_value), details::to_string(expected_value), location);
    }

    template<class a_type, class b_type>
    void assert_bigger_equal(const a_type& given_value,
                             const b_type& expected_value,
                             const std::source_location location = std::source_location::current()) {
        if (given_value >= expected_value) {
            return;
        }

        details::assert_bigger_equal_failed_impl(details::to_string(given_value), details::to_string(expected_value), location);
    }

    template<class a_type, class b_type>
    void assert_smaller_equal(const a_type& given_value,
                              const b_type& expected_value,
                              const std::source_location location = std::source_location::current()) {
        if (given_value <= expected_value) {
            return;
        }

        details::assert_smaller_equal_failed_impl(details::to_string(given_value), details::to_string(expected_value), location);
    }

    template<class exception_type, class task_type>
    void assert_throws(task_type&& task, const std::source_location location = std::source_location::current()) {
        std::stringstream stream;
        auto exception_thrown = false;

        try {
            task();
        } catch (const exception_type&) {
            return;
        } catch (...) {
            stream << "assert_throws failed: caught a different exception type.";
            exception_thrown = true;
        }

        if (!exception_thrown) {
            stream << "assert_throws failed: no exception was thrown.";
        }

        details::add_source_info_msg(stream, location);
        details::log_error_and_abort(stream);
    }

    template<class exception_type, class task_type>
    void assert_throws_with_error_message(task_type&& task,
                                          std::string_view error_msg,
                                          const std::source_location location = std::source_location::current()) {
        std::stringstream stream;
        auto exception_thrown = false;

        try {
            task();
        } catch (const exception_type& e) {
            if (error_msg == e.what()) {
                return;
            }

            stream << "assert_throws_with_error_message failed: error messages don't match, excpected \"" << error_msg << "\" got: \""
                   << e.what() << "\"";
            exception_thrown = true;
        } catch (...) {
            stream << "assert_throws_with_error_message failed: caught a different exception type.";
            exception_thrown = true;
        }

        if (!exception_thrown) {
            stream << "assert_throws_with_error_message failed: no exception was thrown.";
        }

        details::add_source_info_msg(stream, location);
        details::log_error_and_abort(stream);
    }

    template<class exception_type, class task_type>
    void assert_throws_contains_error_message(task_type&& task,
                                              std::string_view error_msg,
                                              const std::source_location location = std::source_location::current()) {
        std::stringstream stream;
        auto exception_thrown = false;

        try {
            task();
        } catch (const exception_type& e) {
            const auto pos = std::string(e.what()).find(error_msg);
            if (pos != std::string::npos) {
                return; 
            }
            
            stream << "assert_throws_contains_error_message failed: error message wasn't found, sub-message: \"" << error_msg << "\" got: \""
                   << e.what() << "\"";
            exception_thrown = true;
        } catch (...) {
            stream << "assert_throws_contains_error_message failed: caught a different exception type.";
            exception_thrown = true;
        }

        if (!exception_thrown) {
            stream << "assert_throws_contains_error_message failed: no exception was thrown.";
        }

        details::add_source_info_msg(stream, location);
        details::log_error_and_abort(stream);
    }

}  // namespace concurrencpp::tests

#endif  // CONCURRENCPP_ASSERTIONS_H
