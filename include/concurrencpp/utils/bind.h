#ifndef CONCURRENCPP_BIND_H
#define CONCURRENCPP_BIND_H

#include <tuple>
#include <type_traits>

namespace concurrencpp::details {
	template<class callable_type, class ... argument_types>
	auto bind(callable_type&& callable, argument_types&& ... arguments) {
		constexpr static auto inti = std::is_nothrow_invocable_v<callable_type, argument_types...>;
		return[
			callable = std::forward<callable_type>(callable),
			tuple = std::make_tuple(std::forward<argument_types>(arguments)...)
		]() mutable noexcept(inti) -> decltype(auto) {
			return std::apply(callable, tuple);
		};
	}
}

#endif