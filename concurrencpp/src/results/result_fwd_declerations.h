#ifndef CONCURRENCPP_RESULT_FWD_DECLERATIONS_H
#define CONCURRENCPP_RESULT_FWD_DECLERATIONS_H

#include "../forward_declerations.h"

#include <memory>
#include <utility>
#include <experimental/coroutine>

namespace concurrencpp {
	template<class type> class result;
	template<class type> class result_promise;

	template<class type> class awaitable;
	template<class type> class via_awaitable;
	template<class type> class resolve_awaitable;
	template<class type> class resolve_via_awaitable;

	struct executor_tag {};

	struct null_result {};

	enum class result_status {
		idle,
		value,
		exception
	};
}

namespace concurrencpp::details {
	template<class type>
	class result_core;

	template<class type>
	class coroutine_state;

	template<class type>
	using coro_handle = std::experimental::coroutine_handle<coroutine_state<type>>;

	using await_context = std::pair<std::shared_ptr<executor>, std::experimental::coroutine_handle<>>;

	struct executor_bulk_tag {};
}

#endif