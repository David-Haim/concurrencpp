#ifndef CONCURRENCPP_BIND_H
#define CONCURRENCPP_BIND_H

#include <tuple>
#include <type_traits>

namespace concurrencpp::details {
    template<class callable_type>
    auto&& bind(callable_type&& callable) {
        return std::forward<callable_type>(callable);  // no arguments to bind
    }

    template<class callable_type, class... argument_types>
    auto bind(callable_type&& callable, argument_types&&... arguments) {
        constexpr static auto inti = std::is_nothrow_invocable_v<callable_type, argument_types...>;
        return [callable = std::forward<callable_type>(callable),
                tuple = std::make_tuple(std::forward<argument_types>(arguments)...)]() mutable noexcept(inti) -> decltype(auto) {
            return std::apply(callable, tuple);
        };
    }

    template<class callable_type>
    auto&& bind_with_try_catch_impl(std::true_type /*is_noexcept*/, callable_type&& callable) {
        return std::forward<callable_type>(callable);
    }

    template<class callable_type>
    auto bind_with_try_catch_impl(std::false_type /*is_noexcept*/, callable_type&& callable) {
        return [callable = std::forward<callable_type>(callable)]() mutable noexcept {
            try {
                callable();
            } catch (...) {
                // do nothing
            }
        };  // no arguments to bind
    }

    template<class callable_type>
    auto bind_with_try_catch(callable_type&& callable) {
        using is_noexcept = typename std::is_nothrow_invocable<callable_type>::type;
        return bind_with_try_catch_impl(is_noexcept {}, std::forward<callable_type>(callable));
    }

    template<class callable_type, class... argument_types>
    auto bind_with_try_catch(callable_type&& callable, argument_types&&... arguments) {
        return bind_with_try_catch(bind(std::forward<callable_type>(callable), std::forward<argument_types>(arguments)...));
    }
}  // namespace concurrencpp::details

#endif