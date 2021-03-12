#ifndef CONCURRENCPP_CUSTOM_EXCEPTION_H
#define CONCURRENCPP_CUSTOM_EXCEPTION_H

#include <numeric>
#include <exception>

namespace concurrencpp::tests {
    struct custom_exception : public std::exception {
        const intptr_t id;

        custom_exception(intptr_t id) noexcept : id(id) {}
    };
}  // namespace concurrencpp::tests

#endif
