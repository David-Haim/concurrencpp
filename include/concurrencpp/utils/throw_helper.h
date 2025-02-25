#ifndef CONCURRENCPP_THROW_HELPER_H
#define CONCURRENCPP_THROW_HELPER_H

#include <string_view>

#include "concurrencpp/errors.h"

namespace concurrencpp::details {
    struct throw_helper {

        template<class exception_type, class state_type>
        static void throw_if_empty_object(const state_type& state, std::string_view class_name, const char* method) {
            if (!static_cast<bool>(state)) [[unlikely]] {
                throw make_empty_object_exception<exception_type>(class_name, method);
            }
        }

        template<class state_type>
        static void throw_if_null_argument(const state_type& state,
                                           std::string_view class_name,
                                           const char* method,
                                           const char* arg_name) {
            if (!static_cast<bool>(state)) [[unlikely]] {
                throw make_empty_argument_exception(class_name, method, arg_name);
            }
        }

        template<class state_type>
        [[noreturn]] static void throw_worker_shutdown_exception(std::string_view class_name, const char* method) {
            throw make_worker_shutdown_exception(class_name, method);
        }

        template<class exception_type>
        static exception_type make_empty_object_exception(std::string_view class_name, const char* method) {
            char buffer[256];
            std::snprintf(buffer,
                          std::size(buffer),
                          "concurrencpp::%s::%s() - %s is empty.",
                          class_name.data(),
                          method,
                          class_name.data());
            return exception_type(buffer);
        }

        static std::invalid_argument make_empty_argument_exception(std::string_view class_name,
                                                                   const char* method,
                                                                   const char* arg_name) {
            char buffer[256];
            std::snprintf(buffer,
                          std::size(buffer),
                          "concurrencpp::%s::%s() - given %s is null or empty.",
                          class_name.data(),
                          method,
                          class_name.data());
            return std::invalid_argument(buffer);
        }

        static errors::runtime_shutdown make_worker_shutdown_exception(std::string_view class_name,
                                                                   const char* method) {
            char buffer[256];
            std::snprintf(buffer,
                          std::size(buffer),
                          "concurrencpp::%s::%s() - %s has already been shut down.",
                          class_name.data(),
                          method,
                          class_name.data());
            return errors::runtime_shutdown(buffer);
        }
    };
}  // namespace concurrencpp::details

#endif
