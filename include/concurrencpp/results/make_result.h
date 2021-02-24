#ifndef CONCURRENCPP_MAKE_RESULT_H
#define CONCURRENCPP_MAKE_RESULT_H

#include "concurrencpp/results/result.h"

namespace concurrencpp {
    template<class type, class... argument_types>
    result<type> make_ready_result(argument_types&&... arguments) {
        static_assert(std::is_constructible_v<type, argument_types...> || std::is_same_v<type, void>,
                      "concurrencpp::make_ready_result - <<type>> is not constructible from <<argument_types...>");

        static_assert(std::is_same_v<type, void> ? (sizeof...(argument_types) == 0) : true,
                      "concurrencpp::make_ready_result<void> - this overload does not accept any argument.");

        details::producer_result_state_ptr<type> promise(new details::result_state<type>());
        details::consumer_result_state_ptr<type> state_ptr(promise.get());

        promise->set_result(std::forward<argument_types>(arguments)...);
        promise.reset();  // publish the result;

        return {std::move(state_ptr)};
    }

    template<class type>
    result<type> make_exceptional_result(std::exception_ptr exception_ptr) {
        if (!static_cast<bool>(exception_ptr)) {
            throw std::invalid_argument(details::consts::k_make_exceptional_result_exception_null_error_msg);
        }

        details::producer_result_state_ptr<type> promise(new details::result_state<type>());
        details::consumer_result_state_ptr<type> state_ptr(promise.get());

        promise->set_exception(exception_ptr);
        promise.reset();  // publish the result;

        return {std::move(state_ptr)};
    }

    template<class type, class exception_type>
    result<type> make_exceptional_result(exception_type exception) {
        return make_exceptional_result<type>(std::make_exception_ptr(exception));
    }
}  // namespace concurrencpp

#endif