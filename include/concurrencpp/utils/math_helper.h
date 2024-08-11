#ifndef CONCURRENCPP_MATH_H
#define CONCURRENCPP_MATH_H

#include "concurrencpp/platform_defs.h"

#include <cstddef>

namespace concurrencpp::details {
    struct CRCPP_API math_helper {
        static bool is_prime(size_t n) noexcept;
        static size_t next_prime(size_t n) noexcept;
    };
}  // namespace concurrencpp::details

#endif