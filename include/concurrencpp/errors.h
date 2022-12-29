#ifndef CONCURRENCPP_ERRORS_H
#define CONCURRENCPP_ERRORS_H

#include <stdexcept>

namespace concurrencpp::errors {
    struct CRCPP_API empty_object : public std::runtime_error {
        using runtime_error::runtime_error;
    };

    struct CRCPP_API empty_result : public empty_object {
        using empty_object::empty_object;
    };

    struct CRCPP_API empty_result_promise : public empty_object {
        using empty_object::empty_object;
    };

    struct CRCPP_API empty_awaitable : public empty_object {
        using empty_object::empty_object;
    };

    struct CRCPP_API empty_timer : public empty_object {
        using empty_object::empty_object;
    };

    struct CRCPP_API empty_generator : public empty_object {
        using empty_object::empty_object;
    };

    struct CRCPP_API interrupted_task : public std::runtime_error {
        using runtime_error::runtime_error;
    };

    struct CRCPP_API broken_task : public interrupted_task {
        using interrupted_task::interrupted_task;
    };

    struct CRCPP_API runtime_shutdown : public interrupted_task {
        using interrupted_task::interrupted_task;
    };

    struct CRCPP_API result_already_retrieved : public std::runtime_error {
        using runtime_error::runtime_error;
    };
}  // namespace concurrencpp::errors

#endif  // ERRORS_H
