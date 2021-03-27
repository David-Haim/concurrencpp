#ifndef CONCURRENCPP_ERRORS_H
#define CONCURRENCPP_ERRORS_H

#include <string>
#include <stdexcept>

namespace concurrencpp::errors {
    struct empty_object : public std::runtime_error {
        empty_object(const std::string& message) : runtime_error(message) {}
    };

    struct empty_result : public empty_object {
        empty_result(const std::string& message) : empty_object(message) {}
    };

    struct empty_result_promise : public empty_object {
        empty_result_promise(const std::string& message) : empty_object(message) {}
    };

    struct empty_awaitable : public empty_object {
        empty_awaitable(const std::string& message) : empty_object(message) {}
    };

    struct empty_timer : public empty_object {
        empty_timer(const std::string& error_messgae) : empty_object(error_messgae) {}
    };

    struct broken_task : public std::runtime_error {
        broken_task(const std::string& message) : runtime_error(message) {}
    };

    struct result_already_retrieved : public std::runtime_error {
        result_already_retrieved(const std::string& message) : runtime_error(message) {}
    };

    struct runtime_shutdown : public std::runtime_error {
        runtime_shutdown(const std::string& message) : runtime_error(message) {}
    };
}  // namespace concurrencpp::errors

#endif  // ERRORS_H
