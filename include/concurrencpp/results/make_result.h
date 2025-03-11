#ifndef CONCURRENCPP_MAKE_RESULT_H
#define CONCURRENCPP_MAKE_RESULT_H

#include "concurrencpp/results/result.h"
#include "concurrencpp/results/lazy_result.h"

namespace concurrencpp::details {
    template<class type>
    lazy_result<type> make_ready_lazy_result_impl(type t) {
        co_return std::move(t);
    }

    template<class type>
    lazy_result<type> make_exceptional_lazy_result_impl(std::exception_ptr exception_ptr) {
        assert(static_cast<bool>(exception_ptr));
        std::rethrow_exception(exception_ptr);
        co_await suspend_never {};
    }
}  // namespace concurrencpp::details

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
        details::throw_helper::throw_if_null_argument(exception_ptr, "", "make_exceptional_result", "exception_ptr");

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

    template<class type, class... argument_types>
    lazy_result<type> make_ready_lazy_result(argument_types&&... arguments) {
        static_assert(!std::is_same_v<type, void>,
                      "concurrencpp::make_ready_lazy_result<void> - void overload does not accept any arguments.");

        static_assert(std::is_constructible_v<type, argument_types...>,
                      "concurrencpp::make_ready_lazy_result - <<type>> is not constructible from <<argument_types...>");

        return details::make_ready_lazy_result_impl<type>(type {std::forward<argument_types>(arguments)...});
    }

    template<>
    inline lazy_result<void> make_ready_lazy_result<void>() {
        co_return;
    }

    template<class type>
    lazy_result<type&> make_ready_lazy_result(type& t) {
        co_return std::ref(t);
    }

    template<class type, class exception_type>
    lazy_result<type> make_exceptional_lazy_result(exception_type exception) {
        throw exception;
        co_await details::suspend_never {};
    }

    template<class type>
    lazy_result<type> make_exceptional_lazy_result(std::exception_ptr exception_ptr) {
        details::throw_helper::throw_if_null_argument(exception_ptr, "", "make_exceptional_lazy_result", "exception_ptr");
        return details::make_exceptional_lazy_result_impl<type>(exception_ptr);
    }
}  // namespace concurrencpp

#endif