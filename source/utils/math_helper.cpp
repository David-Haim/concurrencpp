#include "concurrencpp/utils/math_helper.h"

using concurrencpp::details::math_helper;

bool math_helper::is_prime(size_t n) noexcept {
    if (n <= 1) {
        return false;
    }

    if (n <= 3) {
        return true;
    }
    if (n % 2 == 0 || n % 3 == 0) {
        return false;
    }

    for (size_t i = 5; i * i <= n; i += 6) {
        if (n % i == 0 || n % (i + 2) == 0) {
            return false;
        }
    }

    return true;
}

size_t math_helper::next_prime(size_t n) noexcept {
    if (n <= 1) {
        return 2;
    }

    while (!is_prime(n)) {
        n++;
    }

    return n;
}

