#include "concurrencpp/utils/throw_helper.h"

using concurrencpp::details::throw_helper;

std::invalid_argument throw_helper::make_empty_argument_exception(std::string_view class_name,
                                                                  const char* method,
                                                                  const char* arg_name) {
    assert(method != nullptr && std::strlen(method) != 0);
    assert(arg_name != nullptr && std::strlen(arg_name) != 0);

    char buffer[256];
    if (class_name.empty()) {
        std::snprintf(buffer, std::size(buffer), "concurrencpp::%s() - given %s is null or empty.", method, arg_name);
    } else {
        std::snprintf(buffer,
                      std::size(buffer),
                      "concurrencpp::%s::%s() - given %s is null or empty.",
                      class_name.data(),
                      method,
                      arg_name);
    }

    return std::invalid_argument(buffer);
}

concurrencpp::errors::runtime_shutdown throw_helper::make_worker_shutdown_exception(std::string_view class_name, const char* method) {
    assert(class_name.data() != nullptr);
    assert(method != nullptr && std::strlen(method) != 0);

    char buffer[256];
    std::snprintf(buffer,
                  std::size(buffer),
                  "concurrencpp::%s::%s() - %s has already been shut down.",
                  class_name.data(),
                  method,
                  class_name.data());

    return errors::runtime_shutdown(buffer);
}
