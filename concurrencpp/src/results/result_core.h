#ifndef CONCURRENCPP_RESULT_CORE_H
#define CONCURRENCPP_RESULT_CORE_H

#include "executor_exception.h"
#include "result_fwd_declerations.h"

#include <mutex>
#include <vector>
#include <atomic>
#include <utility>
#include <variant>
#include <type_traits>
#include <condition_variable>

#include <cassert>

namespace concurrencpp::details {
	struct result_core_per_thread_data {
		executor* executor = nullptr;
		std::vector<std::experimental::coroutine_handle<>>* accumulator = nullptr;

		static thread_local result_core_per_thread_data s_tl_per_thread_data;
	};

	class wait_context {

	private:
		std::mutex m_lock;
		std::condition_variable m_condition;
		bool m_ready = false;

	public:
		void wait() noexcept;
		bool wait_for(size_t milliseconds) noexcept;

		void notify() noexcept;

		static std::shared_ptr<wait_context> make();
	};
}

namespace concurrencpp::details {
	template<class type>
	class async_result {

	public:
		using result_context = std::variant<std::monostate, type, std::exception_ptr>;

	protected:
		result_context m_result;

		template<class ... argument_types>
		void build_impl(std::false_type /*no_throw*/, argument_types&& ... arguments) {
			try {
				m_result.template emplace<1>(std::forward<argument_types>(arguments)...);
			}
			catch (...) {
				assert(m_result.index() == -1);
				m_result.template emplace<0>();
				throw;
			}
		}

		template<class ... argument_types>
		void build_impl(std::true_type /*no_throw*/, argument_types&& ... arguments) noexcept {
			m_result.template emplace<1>(std::forward<argument_types>(arguments)...);
		}

	public:
		template<class ... argument_types>
		void build(argument_types&& ... arguments) {
			using intc = std::is_nothrow_constructible<type, argument_types...>;
			build_impl(intc(), std::forward<argument_types>(arguments)...);
		}

		type get() {
			const auto index = m_result.index();
			assert(index != 0);

			if (index == 2) {
				std::rethrow_exception(std::get<2>(m_result));
			}

			return std::move(std::get<1>(m_result));
		}
	};

	template<>
	class async_result<void> {

	public:
		using result_context = std::variant<std::monostate, std::monostate, std::exception_ptr>;

	protected:
		result_context m_result;

	public:
		void build() noexcept {
			m_result.template emplace<1>();
		}

		void get() {
			const auto index = m_result.index();
			assert(index != 0);

			if (index == 2) {
				std::rethrow_exception(std::get<2>(m_result));
			}
		}
	};

	template<class type>
	class async_result<type&> {

	public:
		using result_context = std::variant<std::monostate, type*, std::exception_ptr>;

	protected:
		result_context m_result;

	public:
		void build(type& reference) noexcept {
			m_result.template emplace<1>(std::addressof(reference));
		}

		type& get() {
			const auto index = m_result.index();
			assert(index != 0);

			if (index == 2) {
				std::rethrow_exception(std::get<2>(m_result));
			}

			auto pointer = std::get<1>(m_result);
			assert(pointer != nullptr);
			assert(reinterpret_cast<size_t>(pointer) % alignof(type) == 0);

			return *pointer;
		}
	};

	class result_core_base {

	public:
		using consumer_context = std::variant<
			std::monostate,
			std::experimental::coroutine_handle<>,
			await_context,
			std::shared_ptr<wait_context>>;

		enum class pc_state {
			idle,
			producer,
			consumer
		};

	protected:
		consumer_context m_consumer;
		std::atomic<pc_state> m_pc_state;

		void assert_consumer_idle() const noexcept {
			assert(m_consumer.index() == 0);
		}

		void assert_done() const noexcept {
			assert(m_pc_state.load(std::memory_order_relaxed) == pc_state::producer);
		}

		void clear_consumer() noexcept {
			m_consumer.template emplace<0>();
		}

		static void schedule_coroutine(executor& executor, std::experimental::coroutine_handle<> handle);
		static void schedule_coroutine(await_context& await_ctx);

	public:
		void wait();

		bool await(std::experimental::coroutine_handle<> caller_handle) noexcept;

		bool await_via(
			std::shared_ptr<concurrencpp::executor> executor,
			std::experimental::coroutine_handle<> caller_handle,
			bool force_rescheduling);

		static bool initial_reschedule(std::experimental::coroutine_handle<> handle);
	};

	template<class type>
	class result_core : public async_result<type>, public result_core_base {

		using result_context = typename async_result<type>::result_context;
		using consumer_context = typename result_core_base::consumer_context;

	private:
		void assert_producer_idle() const noexcept {
			assert(this->m_result.index() == 0);
		}

		void clear_producer() noexcept {
			this->m_result.template emplace<0>();
		}

		void schedule_continuation(await_context& await_ctx) noexcept {
			try {
				this->schedule_coroutine(await_ctx);
			}
			catch (...) {
				auto executor_error = std::make_exception_ptr(
					errors::executor_exception(
						std::current_exception(),
						std::move(await_ctx.first)));

				//consumer can't interfere here
				clear_producer();
				this->set_exception(executor_error);
				await_ctx.second();
			}
		}

		template<class callable_type>
		void from_callable(std::true_type /*is_void_type*/, callable_type&& callable) {
			callable();
			this->set_result();
		}

		template<class callable_type>
		void from_callable(std::false_type /*is_void_type*/, callable_type&& callable) {
			this->set_result(callable());
		}

	public:
		template<class ... argument_types>
		void set_result(argument_types&& ... arguments) {
			this->assert_producer_idle();
			this->build(std::forward<argument_types>(arguments)... );
		}

		void set_exception(std::exception_ptr error) noexcept {
			assert(error != nullptr);
			this->assert_producer_idle();
			this->m_result.template emplace<2>(std::move(error));
		}

		//Consumer-side functions
		result_status status() const noexcept {
			const auto state = this->m_pc_state.load(std::memory_order_acquire);
			assert(state != pc_state::consumer);

			if (state == pc_state::idle) {
				return result_status::idle;
			}

			const auto index = this->m_result.index();
			if (index == 1) {
				return result_status::value;
			}

			assert(index == 2);
			return result_status::exception;
		}

		template<class duration_unit, class ratio>
		concurrencpp::result_status wait_for(std::chrono::duration<duration_unit, ratio> duration) {
			auto get_result_status = [this] {
				return this->m_result.index() == 1 ?
					result_status::value : result_status::exception;
			};

			const auto state_0 = this->m_pc_state.load(std::memory_order_acquire);
			if (state_0 == pc_state::producer) {
				return get_result_status();
			}

			assert_consumer_idle();

			auto wait_ctx = wait_context::make();
			m_consumer.template emplace<3>(wait_ctx);

			auto expected_idle_state = pc_state::idle;
			const auto idle_0 = this->m_pc_state.compare_exchange_strong(
				expected_idle_state,
				pc_state::consumer,
				std::memory_order_acq_rel);

			if (!idle_0) {
				assert_done();
				return get_result_status();
			}

			const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
			if (wait_ctx->wait_for(static_cast<size_t>(ms + 1))) {
				assert_done();
				return get_result_status();
			}

			/*
				now we need to rewind what we've done: the producer might try to access
				the consumer context while we rewind the consumer context back to nothing.
				first we'll cas the status back to idle. if we failed - the producer has set its result,
				then there's no point in continue rewinding - we just return the status of the result.
				if we managed to rewind the status back to idle,
				then the consumer is "protected" because the producer will not try
				to access the consumer if the flag doesn't say so.
			*/
			auto expected_consumer_state = pc_state::consumer;
			const auto idle_1 = this->m_pc_state.compare_exchange_strong(
				expected_consumer_state,
				pc_state::idle,
				std::memory_order_acq_rel);

			if (!idle_1) {
				assert_done();
				return get_result_status();
			}

			m_consumer.template emplace<0>();
			return result_status::idle;
		}

		template<class clock, class duration>
		concurrencpp::result_status wait_until(const std::chrono::time_point<clock, duration>& timeout_time) {
			const auto now = clock::now();
			if (timeout_time <= now) {
				return status();
			}

			const auto diff = timeout_time - now;
			return wait_for(diff);
		}

		type get() {
			assert_done();
			return this->async_result<type>::get();
		}

		void publish_result() noexcept {
			const auto state_before = this->m_pc_state.exchange(pc_state::producer, std::memory_order_acq_rel);
			if (state_before == pc_state::idle) {
				return;
			}

			assert(state_before == pc_state::consumer);

			switch (this->m_consumer.index())
			{
			case 0: {
				return;
			}

			case 1: {
				auto handle = std::get<1>(this->m_consumer);
				handle();
				return;
			}

			case 2: {
				auto await_ctx = std::move(std::get<2>(this->m_consumer));
				return this->schedule_continuation(await_ctx);
			}

			case 3: {
				const auto wait_ctx = std::move(std::get<3>(this->m_consumer));
				wait_ctx->notify();
				return;
			}

			default:
				break;
			}

			assert(false);
		}

		template<class callable_type>
		void from_callable(callable_type&& callable) {
			using is_void = std::is_same<type, void>;

			try {
				this->from_callable(is_void{}, std::forward<callable_type>(callable));
			}
			catch (...) {
				this->set_exception(std::current_exception());
			}
		}
	};
}

#endif