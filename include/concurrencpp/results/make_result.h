#ifndef CONCURRENCPP_MAKE_RESULT_H
#define CONCURRENCPP_MAKE_RESULT_H

#include "concurrencpp/results/result.h"

namespace concurrencpp {
    template<class type, class... argument_types>
    result<type> make_ready_result(argument_types&&... arguments) {
        static_assert(std::is_constructible_v<type, argument_types...> || std::is_same_v<type, void>,
                      "concurrencpp::make_ready_result - <<type>> is not constructible from <<argument_types...>");

        auto result_state_ptr = std::make_shared<details::result_state<type>>();
        result_state_ptr->set_result(std::forward<argument_types>(arguments)...);
        result_state_ptr->publish_result();
        return {std::move(result_state_ptr)};
    }

    inline result<void> make_ready_result() {
        auto result_state_ptr = std::make_shared<details::result_state<void>>();
        result_state_ptr->set_result();
        result_state_ptr->publish_result();
        return {std::move(result_state_ptr)};
    }

    template<class type>
    result<type> make_exceptional_result(std::exception_ptr exception_ptr) {
        if (!static_cast<bool>(exception_ptr)) {
            throw std::invalid_argument(details::consts::k_make_exceptional_result_exception_null_error_msg);
        }

        auto result_state_ptr = std::make_shared<details::result_state<type>>();
        result_state_ptr->set_exception(exception_ptr);
        result_state_ptr->publish_result();
        return {std::move(result_state_ptr)};
    }

    template<class type, class exception_type>
    result<type> make_exceptional_result(exception_type exception) {
        return make_exceptional_result<type>(std::make_exception_ptr(exception));
    }
}  // namespace concurrencpp

#endif
