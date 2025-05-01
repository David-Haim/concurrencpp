#ifndef CONCURRENCPP_THROW_HELPER_H
#define CONCURRENCPP_THROW_HELPER_H

#include "concurrencpp/errors.h"

#include <string_view>

#include <cstring>
#include <cassert>

namespace concurrencpp::details {
    struct CRCPP_API throw_helper {

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

        [[noreturn]] static void throw_worker_shutdown_exception(std::string_view class_name, const char* method) {
            throw make_worker_shutdown_exception(class_name, method);
        }

        template<class exception_type>
        static exception_type make_empty_object_exception(std::string_view class_name, const char* method) {
            assert(class_name.data() != nullptr);
            assert(method != nullptr && std::strlen(method) != 0);
            
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
                                                                   const char* arg_name);

        static errors::runtime_shutdown make_worker_shutdown_exception(std::string_view class_name, const char* method);
    };
}  // namespace concurrencpp::details

#endif
