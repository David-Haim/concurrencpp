#include "concurrencpp/io/impl/win32/utils.h"

#include <cassert>

std::error_code concurrencpp::details::make_error_code(uint32_t code) {
    return {static_cast<int>(code), std::system_category()};
}

std::system_error concurrencpp::details::make_system_error(uint32_t error) {
    return std::system_error(error, std::system_category());
}

void concurrencpp::details::throw_system_error(uint32_t error) {
    throw make_system_error(error);
}