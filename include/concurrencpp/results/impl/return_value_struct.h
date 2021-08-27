#ifndef CONCURRENCPP_RETURN_VALUE_STRUCT_H
#define CONCURRENCPP_RETURN_VALUE_STRUCT_H

#include <utility>

namespace concurrencpp::details {
    template<class derived_type, class type>
    struct return_value_struct {
        template<class return_type>
        void return_value(return_type&& value) {
            auto self = static_cast<derived_type*>(this);
            self->set_result(std::forward<return_type>(value));
        }
    };

    template<class derived_type>
    struct return_value_struct<derived_type, void> {
        void return_void() noexcept {
            auto self = static_cast<derived_type*>(this);
            self->set_result();
        }
    };
}  // namespace concurrencpp::details

#endif