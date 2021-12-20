#ifndef CONCURRENCPP_ERRORS_H
#define CONCURRENCPP_ERRORS_H

#include <stdexcept>

namespace concurrencpp::errors {
    struct empty_object : public std::runtime_error {
        using runtime_error::runtime_error;
    };

    struct empty_result : public empty_object {
        using empty_object::empty_object;
    };

    struct empty_result_promise : public empty_object {
        using empty_object::empty_object;
    };

    struct empty_awaitable : public empty_object {
        using empty_object::empty_object;
    };

    struct empty_timer : public empty_object {
        using empty_object::empty_object;
    };

    struct empty_generator : public empty_object {
        using empty_object::empty_object;
    };

    struct broken_task : public std::runtime_error {
        using runtime_error::runtime_error;
    };

    struct result_already_retrieved : public std::runtime_error {
        using runtime_error::runtime_error;
    };

    struct runtime_shutdown : public std::runtime_error {
        using runtime_error::runtime_error;
    };
}  // namespace concurrencpp::errors

#endif  // ERRORS_H
