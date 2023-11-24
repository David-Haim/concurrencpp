#ifndef CONCURRENCPP_THROW_HELPER_H
#define CONCURRENCPP_THROW_HELPER_H

#include "concurrencpp/errors.h"

namespace concurrencpp::details {
    struct throw_helper {

        template<class exception_type, class state_type>
        static void throw_if_empty_object(const state_type& state, const char* class_name, const char* method) {
            if (static_cast<bool>(state)) [[likely]] {
                return;
            }

            throw make_empty_object_exception<exception_type>(class_name, method);
        }

        template<class state_type>
        static void throw_if_null_argument(const state_type& state, const char* class_name, const char* method, const char* arg_name) {
            if (static_cast<bool>(state)) [[likely]] {
                return;
            }

            throw make_empty_argument_exception(class_name, method, arg_name);
        }

        template<class exception_type>
        static exception_type make_empty_object_exception(const char* class_name, const char* method) {
            std::string error_msg = std::string("concurrencpp::") + class_name + "::" + method + "() - " + class_name + " is empty.";
            return exception_type(error_msg.c_str());
        }

        static std::invalid_argument make_empty_argument_exception(const char* class_name, const char* method, const char* arg_name) {
            std::string error_msg =
                std::string("concurrencpp::") + class_name + "::" + method + "() - given " + arg_name + " is null or empty.";
            return std::invalid_argument(error_msg.c_str());
        }
    };
}  // namespace concurrencpp::details

#endif
