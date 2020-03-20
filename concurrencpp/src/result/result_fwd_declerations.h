#ifndef CONCURRENCPP_RESULT_FWD_DECLERATIONS_H
#define CONCURRENCPP_RESULT_FWD_DECLERATIONS_H

#include <experimental/coroutine>
#include <memory>

namespace concurrencpp {
	template<class type> class result;
	template<class type> class result_promise;

	template<class type> struct result_awaitable;
	template<class type> struct result_resolver;

	enum class result_status {
		IDLE,
		VALUE,
		EXCEPTION
	};
}

namespace concurrencpp::details {
	template<class type> class result_storage;
	template<class type> class result_state;

	template<class type>
	struct result_state_ptr_deleter {
		void operator()(result_state<type>* state) const noexcept;
	};
}

namespace concurrencpp::details {
	template<class type> 
	using result_state_ptr = std::unique_ptr<result_state<type>, result_state_ptr_deleter<type>>;
	using void_coro_handle = std::experimental::coroutine_handle<void>;
}

#endif