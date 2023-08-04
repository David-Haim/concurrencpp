#include "concurrencpp/forward_declarations.h"
#include "infra/assertions.h"

#include <iostream>
#include <string>
#include <string_view>

#include <cassert>

namespace concurrencpp::tests::details {
    std::string to_string(bool value) {
        return value ? "true" : "false";
    }

    std::string to_string(int value) {
        return std::to_string(value);
    }

    std::string to_string(long value) {
        return std::to_string(value);
    }

    std::string to_string(long long value) {
        return std::to_string(value);
    }

    std::string to_string(unsigned value) {
        return std::to_string(value);
    }

    std::string to_string(unsigned long value) {
        return std::to_string(value);
    }

    std::string to_string(unsigned long long value) {
        return std::to_string(value);
    }

    std::string to_string(float value) {
        return std::to_string(value);
    }

    std::string to_string(double value) {
        return std::to_string(value);
    }

    std::string to_string(long double value) {
        return std::to_string(value);
    }

    const std::string& to_string(const std::string& str) {
        return str;
    }

    std::string to_string(const char* str) {
        return str;
    }

    std::string to_string(const std::string_view str) {
        return {str.begin(), str.end()};
    }

    const char* to_string(result_status status) {
        switch (status) {
            case concurrencpp::result_status::idle: {
                return "result_status::idle";
            }
            case concurrencpp::result_status::value: {
                return "result_status::value";
            }
            case concurrencpp::result_status::exception: {
                return "result_status::exception";
            }
            default: {
                break;
            }
        }

        assert(false);
        return nullptr;
    }

    std::string to_string(std::chrono::milliseconds ms) {
        return std::to_string(ms.count()) + "[ms]";
    }

    time_t steady_clock_to_time_t(std::chrono::steady_clock::time_point t) {
        return std::chrono::system_clock::to_time_t(
            std::chrono::system_clock::now() +
            std::chrono::duration_cast<std::chrono::system_clock::duration>(t - std::chrono::steady_clock::now()));
    }

    std::string to_string(std::chrono::steady_clock::time_point time_point) {
        const auto tp = steady_clock_to_time_t(time_point);
        return std::ctime(&tp);
    }

    std::string to_string(std::chrono::system_clock::time_point time_point) {
        const auto tp = std::chrono::system_clock::to_time_t(time_point);
        return std::ctime(&tp);
    }

    std::string to_string(std::tuple<> tp) {
        return "std::tuple<>";
    }

    std::string to_string(std::thread::id id) {
        std::stringstream string_stream;
        string_stream << id;
        return string_stream.str();
    }

    void add_source_info_msg(std::stringstream& stream, const std::source_location& location) {
        stream << " at " << location.file_name() << '(' << location.line() << ':' << location.column()
               << ") :: " << location.function_name();
    }

    void log_error_and_abort(std::stringstream& stream) {
        std::cerr << stream.str() << std::endl;
        std::abort();
    }

    void assert_equal_failed_impl(const std::string& a, const std::string& b, const std::source_location& location) {
        std::stringstream stream;
        stream << "assertion failed. expected [" << a << "] == [" << b << "]";
        add_source_info_msg(stream, location);
        log_error_and_abort(stream);
    }

    void assert_not_equal_failed_impl(const std::string& a, const std::string& b, const std::source_location& location) {
        std::stringstream stream;
        stream << "assertion failed. expected [" << a << "] =/= [" << b << "]";
        add_source_info_msg(stream, location);
        log_error_and_abort(stream);
    }

    void assert_bigger_failed_impl(const std::string& a, const std::string& b, const std::source_location& location) {
        std::stringstream stream;
        stream << "assertion failed. expected [" << a << "] > [" << b << "]";
        add_source_info_msg(stream, location);
        log_error_and_abort(stream);
    }

    void assert_smaller_failed_impl(const std::string& a, const std::string& b, const std::source_location& location) {
        std::stringstream stream;
        stream << "assertion failed. expected [" << a << "] < [" << b << "]";
        add_source_info_msg(stream, location);
        log_error_and_abort(stream);
    }

    void assert_bigger_equal_failed_impl(const std::string& a, const std::string& b, const std::source_location& location) {
        std::stringstream stream;
        stream << "assertion failed. expected [" << a << "] >= [" << b << "]";
        add_source_info_msg(stream, location);
        log_error_and_abort(stream);
    }

    void assert_smaller_equal_failed_impl(const std::string& a, const std::string& b, const std::source_location& location) {
        std::stringstream stream;
        stream << "assertion failed. expected [" << a << "] <= [" << b << "]";
        add_source_info_msg(stream, location);
        log_error_and_abort(stream);
    }
}  // namespace concurrencpp::tests::details

namespace concurrencpp::tests {
    void assert_true(bool condition, const std::source_location location) {
        if (condition) {
            return;
        }

        std::stringstream stream;
        stream << "assertion failed. expected: [true] actual: [false]";
        details::add_source_info_msg(stream, location);
        details::log_error_and_abort(stream);
    }

    void assert_false(bool condition, const std::source_location location) {
        if (!condition) {
            return;
        }

        std::stringstream stream;
        stream << "assertion failed. expected: [false] actual: [true]";
        details::add_source_info_msg(stream, location);
        details::log_error_and_abort(stream);
    }
}  // namespace concurrencpp::tests
