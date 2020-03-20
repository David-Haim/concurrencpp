#ifndef CONCURRENCPP_RESULT_STATE_H
#define CONCURRENCPP_RESULT_STATE_H

#include "../executors/executor.h"
#include "../utils/spinlock.h"
#include "../utils/scope_guard.h"

#include "result_fwd_declerations.h"
#include "executor_exception.h"
#include "result_awaitable.h"

#include <condition_variable>
#include <cassert>

namespace concurrencpp::details {
	template<class type_t>
	class async_value {

	private:
		type_t m_object;

	public:
		template<class ... argument_types>
		async_value(argument_types&& ... arguments) : m_object(std::forward<argument_types>(arguments)...) {}

		type_t get() { return std::move(m_object); }
	};

	template<class type_t>
	class async_value<type_t&> {

	private:
		type_t* m_pointer;

	public:
		async_value(type_t& ref_result) noexcept : m_pointer(std::addressof(ref_result)) {}

		type_t& get() noexcept { return *m_pointer; }
	};

	template<>
	class async_value<void> {

	public:
		void get() noexcept {}
	};

	class await_context {

	private:
		void_coro_handle m_coro;
		std::shared_ptr<concurrencpp::executor> m_executor;

	public:
		await_context(void_coro_handle coro, std::shared_ptr<concurrencpp::executor> executor) noexcept :
			m_coro(coro),
			m_executor(std::move(executor)){}

		await_context(await_context&& rhs) noexcept = default;

		void operator ()() {
			if (!static_cast<bool>(m_executor)) {
				return m_coro();
			}

			m_executor->enqueue(m_coro);
		}

		void resume_inline() noexcept { m_coro(); }

		std::shared_ptr<concurrencpp::executor> get_executor() noexcept { return std::move(m_executor); }
	};

	template<class type>
	class result_storage {

		union compressed_storage {
			async_value<type> result;
			std::exception_ptr error;
			await_context await_ctx;
			std::condition_variable_any* condition;

			compressed_storage() noexcept {}
			~compressed_storage() noexcept {}
		};

	private:
		compressed_storage m_data;

		template<class type_t>
		static void destroy(type_t& object) noexcept { object.~type_t(); }

	public:
		result_storage() noexcept = default;
		~result_storage() noexcept = default;

		result_storage(const result_storage&) = delete;
		result_storage(result_storage&&) = delete;

		template<class ... argument_types>
		void build_value(argument_types&& ... arguments) {
			new (&m_data.result) async_value<type>(std::forward<argument_types>(arguments)...);
		}

		void build_error(std::exception_ptr error) noexcept {
			new (&m_data.error) std::exception_ptr(error);
		}

		void build_condition(std::condition_variable_any& condition) noexcept {
			new (&m_data.condition) std::condition_variable_any* (&condition);
		}

		void build_await_context(void_coro_handle handle, std::shared_ptr<concurrencpp::executor> executor) noexcept {
			new (&m_data.await_ctx) await_context(handle, std::move(executor));
		}

		void build_await_context(await_context await_ctx) noexcept {
			new (&m_data.await_ctx) await_context(std::move(await_ctx));
		}

		await_context get_await_context() noexcept {
			auto scope_guard = make_scope_guard([this] { destroy(m_data.await_ctx); });
			return std::move(m_data.await_ctx);
		}

		std::condition_variable_any& get_condition() noexcept {
			auto scope_guard = make_scope_guard([this] { destroy(m_data.condition); });
			assert(m_data.condition != nullptr);
			assert(reinterpret_cast<std::uintptr_t>(m_data.condition) % alignof(std::condition_variable_any) == 0);
			return *m_data.condition;
		}

		decltype(auto) get_result() { return m_data.result.get(); }
		[[noreturn]] void rethrow_exception() { std::rethrow_exception(m_data.error); }

		void destroy_result() noexcept { destroy(m_data.result); }
		void destroy_error() noexcept { destroy(m_data.error); }
	};
}

namespace concurrencpp::details {
	template<class type>
	class result_state {

		enum class result_state_status : int8_t {
			IDLE,
			VALUE,
			EXCEPTION,
			AWAITED,
			WAITED
		};

		void schedule_continuation(await_context& await_ctx) noexcept {
			assert(m_status == result_state_status::VALUE || m_status == result_state_status::EXCEPTION);

			std::exception_ptr executor_error;

			try {
				return  await_ctx();
			}
			catch (...) {
				auto current_exception = std::current_exception();
				executor_error = std::make_exception_ptr(errors::executor_exception(current_exception, await_ctx.get_executor()));
			}

			set_error_impl(executor_error);
			await_ctx.resume_inline();
		}

		/*
			Note: when resuming, schdeuling or notifying, we must unlock the state lock.
			failing to do so will cause deadlocks.
		*/

		template<class ... argument_types>
		void set_result_impl(argument_types&& ... arguments) {
			m_storage.build_value(std::forward<argument_types>(arguments)...);
			m_status = result_state_status::VALUE;
		}

		void set_error_impl(std::exception_ptr error) noexcept {
			m_storage.build_error(error);
			m_status = result_state_status::EXCEPTION;
		}

		void set_await_context_impl(void_coro_handle coro, std::shared_ptr<concurrencpp::executor> executor) noexcept {
			m_storage.build_await_context(coro, std::move(executor));
			m_status = result_state_status::AWAITED;
		}

		void set_await_context_impl(await_context await_ctx) noexcept {
			m_storage.build_await_context(std::move(await_ctx));
			m_status = result_state_status::AWAITED;
		}

		void set_condition_impl(std::condition_variable_any& condition_ref) noexcept {
			m_storage.build_condition(condition_ref);
			m_status = result_state_status::WAITED;
		}

		template<class ... argument_types>
		void set_result_awaited(std::unique_lock<spinlock> lock, argument_types&& ... arguments) {
			auto await_ctx = m_storage.get_await_context();

			try {
				set_result_impl(std::forward<argument_types>(arguments)...);
			}
			catch (...) {
				set_await_context_impl(std::move(await_ctx));
				throw;
			}

			lock.unlock();
			schedule_continuation(await_ctx);
		}

		template<class ... argument_types>
		void set_result_waited(std::unique_lock<spinlock> lock, argument_types&& ... arguments) {
			assert(m_status == result_state_status::WAITED);
			auto& condition_ref = m_storage.get_condition();

			try {
				set_result_impl(std::forward<argument_types>(arguments)...);
			}
			catch (...) {
				set_condition_impl(condition_ref);
				throw;
			}

			lock.unlock();
			condition_ref.notify_one();
		}

		void set_error_awaited(std::unique_lock<spinlock> lock, std::exception_ptr error) noexcept {
			auto await_ctx = m_storage.get_await_context();
			set_error_impl(error);
			lock.unlock();
			schedule_continuation(await_ctx);
		}

		void set_error_waited(std::unique_lock<spinlock> lock, std::exception_ptr error) noexcept {
			auto& condition_ref = m_storage.get_condition();
			set_error_impl(error);
			lock.unlock();
			condition_ref.notify_one();
		}

		void assert_reader_state() const noexcept {
			//when quering the result object, waiting or awaiting it, it must be either idle, value-ed or exception-ed.
			assert(m_status != result_state_status::WAITED);
			assert(m_status != result_state_status::AWAITED);
		}

		void assert_writer_state() const noexcept {
			//asserting that we don't mistakenly set the result twice.
			assert(m_status != result_state_status::VALUE);
			assert(m_status != result_state_status::EXCEPTION);
		}

	private:
		mutable spinlock m_lock;
		mutable uint8_t m_ref_count;
		result_state_status m_status;
		bool m_result_promise_retreived;
		result_storage<type> m_storage;
		std::experimental::coroutine_handle<> m_destroy_handle;

	public:
		//Memory related functions
		void add_ref() const noexcept {
			std::unique_lock<decltype(m_lock)> lock(m_lock);
			++m_ref_count;
		}

		void dec_ref() noexcept {
			std::unique_lock<decltype(m_lock)> lock(m_lock);
			assert(m_ref_count != 0);
			--m_ref_count;

			if (m_ref_count != 0) {
				return;
			}

			lock.unlock();

			if (static_cast<bool>(m_destroy_handle)) {
				m_destroy_handle.destroy();
				return;
			}

			delete this;
		}

		bool dec_ref_coroutine_step_1() noexcept {
			m_lock.lock();
			assert(m_ref_count != 0);
			--m_ref_count;

			if (m_ref_count == 0) {
				m_lock.unlock();
				return true;
			}

			return false;
		}

		void dec_ref_coroutine_step_2(std::experimental::coroutine_handle<> destroy_handle) noexcept {
			m_destroy_handle = destroy_handle;
			m_lock.unlock();
		}

		result_state_ptr<type> share_state() noexcept {
			add_ref();
			return result_state_ptr<type> { this };
		}

		template<class as_type>
		as_type share_state_as() noexcept {
			static_assert(
				std::is_constructible_v<as_type, result_state_ptr<type>&&>,
				"concurrencpp::details::share_state_as - cannot build <<as_type>> from result_state_ptr<type>");
			return as_type{ share_state() };
		}

	public:
		result_state() noexcept :
			m_ref_count(1),
			m_status(result_state_status::IDLE),
			m_result_promise_retreived(false) {}

		~result_state() noexcept {
			switch (m_status) {

			case result_state_status::VALUE: {
				m_storage.destroy_result();
				return;
			}

			case result_state_status::EXCEPTION: {
				m_storage.destroy_error();
				return;
			}

			default: {
				assert(false);
				return;
			}

			}
		}

		result_state(const result_state& rhs) = delete;
		result_state(result_state&& rhs) = delete;

	public:
		//Writer-side functions
		template<class ... argument_types>
		void set_result(argument_types&& ... arguments) {
			std::unique_lock<decltype(m_lock)> lock(m_lock);
			assert_writer_state();

			switch (m_status) {

			case result_state_status::IDLE: {
				return set_result_impl(std::forward<argument_types>(arguments)...);
			}

			case result_state_status::AWAITED: {
				return set_result_awaited(std::move(lock), std::forward<argument_types>(arguments)...);
			}

			case result_state_status::WAITED: {
				return set_result_waited(std::move(lock), std::forward<argument_types>(arguments)...);
			}

			}

			assert(false);
		}

		void set_exception(std::exception_ptr error) noexcept {
			std::unique_lock<decltype(m_lock)> lock(m_lock);
			assert_writer_state();

			switch (m_status) {

			case result_state_status::IDLE: {
				return set_error_impl(error);
			}

			case result_state_status::AWAITED: {
				return set_error_awaited(std::move(lock), error);
			}

			case result_state_status::WAITED: {
				return set_error_waited(std::move(lock), error);
			}

			}

			assert(false);
		}

		//Reader-side functions
		result_status status() const noexcept {
			std::unique_lock<decltype(m_lock)> lock(m_lock);
			assert_reader_state();

			const auto status = m_status;
			switch (status) {

			case result_state_status::IDLE: {
				return result_status::IDLE;
			}

			case result_state_status::VALUE: {
				return result_status::VALUE;
			}

			case result_state_status::EXCEPTION: {
				return result_status::EXCEPTION;
			}

			default: { break; }
			}

			assert(false);
			return result_status::IDLE;
		}

		void wait() {
			std::unique_lock<decltype(m_lock)> lock(m_lock);
			assert_reader_state();

			if (m_status == result_state_status::VALUE || m_status == result_state_status::EXCEPTION) {
				return;
			}

			assert(m_status == result_state_status::IDLE);

			std::condition_variable_any condition;
			set_condition_impl(condition);

			try {
				condition.wait(lock, [this] { return m_status != result_state_status::WAITED; }); //can throw std::system_error
			}
			catch (...) {
				m_status = result_state_status::IDLE;
				throw;
			}

			assert(m_status == result_state_status::VALUE || m_status == result_state_status::EXCEPTION);
		}

		template<class duration_unit, class ratio>
		concurrencpp::result_status wait_for(std::chrono::duration<duration_unit, ratio> duration) {
			std::unique_lock<decltype(m_lock)> lock(m_lock);
			assert_reader_state();

			if (m_status == result_state_status::VALUE) {
				return result_status::VALUE;
			}

			if (m_status == result_state_status::EXCEPTION) {
				return result_status::EXCEPTION;
			}

			assert(m_status == result_state_status::IDLE);

			std::condition_variable_any condition;
			set_condition_impl(condition);

			try {
				condition.wait_for(lock, duration, [this] { return m_status != result_state_status::WAITED; }); //can throw std::system_error
			}
			catch (...) {
				m_status = result_state_status::IDLE;
				throw;
			}

			if (m_status == result_state_status::VALUE) {
				return result_status::VALUE;
			}

			if (m_status == result_state_status::EXCEPTION) {
				return result_status::EXCEPTION;
			}

			m_status = result_state_status::IDLE;
			return result_status::IDLE;
		}

		template< class clock, class duration>
		concurrencpp::result_status wait_until(const std::chrono::time_point<clock, duration>& timeout_time) {
			const auto now = clock::now();
			if (timeout_time <= now) {
				return status();
			}

			const auto diff = timeout_time - now;
			return wait_for(diff);
		}

		void await(
			std::shared_ptr<concurrencpp::executor> executor,
			void_coro_handle coro_handle,
			bool force_reshceduling) noexcept {

			await_context await_ctx(coro_handle, std::move(executor));

			std::unique_lock<decltype(m_lock)> lock(m_lock);
			assert_reader_state();

			if (m_status == result_state_status::VALUE || m_status == result_state_status::EXCEPTION) {
				lock.unlock();

				if (force_reshceduling) {
					return schedule_continuation(await_ctx);
				}

				return await_ctx.resume_inline();
			}

			assert(m_status == result_state_status::IDLE);
			set_await_context_impl(std::move(await_ctx));
		}

		type get() {
			assert(m_status == result_state_status::VALUE || m_status == result_state_status::EXCEPTION);
			if (m_status == result_state_status::EXCEPTION) {
				m_storage.rethrow_exception();
			}

			return m_storage.get_result();
		}

		bool result_promise_retreived() const noexcept { return m_result_promise_retreived; }
		void result_promise_retreived(bool value) noexcept { m_result_promise_retreived = value; }
	};
}

namespace concurrencpp::details {
	template<class type>
	void result_state_ptr_deleter<type>::operator()(result_state<type>* state) const noexcept {
		if (state != nullptr) {
			state->dec_ref();
		}
	}
}

#endif
