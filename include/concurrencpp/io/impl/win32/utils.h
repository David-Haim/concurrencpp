#pragma once

#include <system_error>

namespace concurrencpp::details {
    std::error_code make_error_code(uint32_t code);

    std::system_error make_system_error(uint32_t error);

    [[noreturn]] void throw_system_error(uint32_t error);
}  // namespace concurrencpp::details
