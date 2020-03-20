#ifndef CONCURRENCPP_EXECUTOR_H
#define CONCURRENCPP_EXECUTOR_H

#include "task.h"
#include "../result/result_fwd_declerations.h"

#include <string_view>

namespace concurrencpp::details {
	template<class functor_type>
	class task_with_result {

		using return_type = typename std::invoke_result_t<functor_type>;
		constexpr static auto intmc = std::is_nothrow_move_constructible_v<functor_type>;

	private:
		result_promise<return_type> m_promise;
		functor_type m_functor;

	public:
		template<class given_functor_type>
		task_with_result(result_promise<return_type> promise, given_functor_type&& functor) :
			m_promise(std::move(promise)),
			m_functor(std::forward<given_functor_type>(functor)){}

		task_with_result(task_with_result&&) noexcept(intmc) = default;

		void operator() () { m_promise.set_from_function(m_functor); }

		void cancel(std::exception_ptr reason) noexcept { m_promise.set_exception(reason); }
	};
}

namespace concurrencpp {
	struct executor {
		virtual ~executor() noexcept = default;

		virtual std::string_view name() const noexcept = 0;
		virtual void enqueue(task task) = 0;

		template<class functor_type>
		auto submit(functor_type&& functor) {
			using decayed_functor_type = typename std::decay_t<functor_type>;
			using return_type = typename std::invoke_result_t<decayed_functor_type>;
			using task_type = details::task_with_result<decayed_functor_type>;

			result_promise<return_type> promise;
			auto result = promise.get_result();
			enqueue(task_type(std::move(promise), std::forward<functor_type>(functor)));
			return result;
		}
	};
}

#endif