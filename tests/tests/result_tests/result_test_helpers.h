#ifndef CONCURRENCPP_RESULT_TEST_HELPERS_H
#define CONCURRENCPP_RESULT_TEST_HELPERS_H

#include "../../helpers/assertions.h"
#include "../../helpers/random.h"

#include "concurrencpp.h"

#include <stdexcept>
#include <type_traits>
#include <string_view>
#include <memory>

namespace concurrencpp::tests {
	template<class type>
	struct result_factory {
		static type get() { return type(); }

		static type throw_ex() {
			throw std::underflow_error("");
			return get();
		}
	};

	template<>
	struct result_factory<int> {
		static int get() { return 123456789; }

		static std::vector<int> get_many(size_t count) {
			std::vector<int> res(count);
			std::iota(res.begin(), res.end(), 0);
			return res;
		}

		static int throw_ex() {
			throw std::underflow_error("");
			return get();
		}
	};

	template<>
	struct result_factory<std::string> {
		static std::string get() { return "abcdefghijklmnopqrstuvwxyz123456789!@#$%^&*()"; }

		static std::vector<std::string> get_many(size_t count) {
			std::vector<std::string> res;
			res.reserve(count);

			for(size_t i = 0 ; i< count; i++) {
				res.emplace_back(std::to_string(i));
			}

			return res;
		}

		static std::string throw_ex() {
			throw std::underflow_error("");
			return get();
		}
	};

	template<>
	struct result_factory<void> {
		static void get() {}

		static void throw_ex() { throw std::underflow_error(""); }

		static std::vector<std::nullptr_t> get_many(size_t count) {
			return std::vector<std::nullptr_t>{ count };
		}
	};

	template<>
	struct result_factory<int&> {
		static int& get() {
			static int i = 0;
			return i;
		}

		static std::vector<std::reference_wrapper<int>> get_many(size_t count) {
			static std::vector<int> s_res(64);
			std::vector<std::reference_wrapper<int>> res;
			res.reserve(count);
			for (size_t i = 0; i < count; i++) {
				res.emplace_back(s_res[i % 64]);
			}

			return res;
		}

		static int& throw_ex() {
			throw std::underflow_error("");
			return get();
		}
	};

	template<>
	struct result_factory<std::string&> {
		static std::string& get() {
			static std::string str;
			return str;
		}

		static std::vector<std::reference_wrapper<std::string>> get_many(size_t count) {
			static std::vector<std::string> s_res(64);
			std::vector<std::reference_wrapper<std::string>> res;
			res.reserve(count);
			for (size_t i = 0; i < count; i++) {
				res.emplace_back(s_res[i % 64]);
			}

			return res;
		}

		static std::string& throw_ex() {
			throw std::underflow_error("");
			return get();
		}
	};
}

namespace concurrencpp::tests {

	template<class type>
	void test_ready_result_result(::concurrencpp::result<type> result, const type& o) {
		assert_true(static_cast<bool>(result));
		assert_equal(result.status(), concurrencpp::result_status::value);

		try
		{
			assert_equal(result.get(), o);
		}
		catch (...) {
			assert_true(false);
		}
	}

	template<class type>
	void test_ready_result_result(
		::concurrencpp::result<type> result,
		std::reference_wrapper<typename std::remove_reference_t<type>> ref) {
		assert_true(static_cast<bool>(result));
		assert_equal(result.status(), concurrencpp::result_status::value);

		try
		{
			assert_equal(&result.get(), &ref.get());
		}
		catch (...) {
			assert_true(false);
		}
	}

	template<class type>
	void test_ready_result_result(::concurrencpp::result<type> result) {
		test_ready_result_result<type>(std::move(result), result_factory<type>::get());
	}

	template<>
	inline void test_ready_result_result(::concurrencpp::result<void> result) {
		assert_true(static_cast<bool>(result));
		assert_equal(result.status(), concurrencpp::result_status::value);
		try
		{
			result.get(); //just make sure no exception is thrown.
		}
		catch (...){
			assert_true(false);
		}
	}

	struct costume_exception : public std::exception {
		const intptr_t id;

		costume_exception(intptr_t id) noexcept : id(id) {}
	};

	template<class type>
	void test_ready_result_costume_exception(concurrencpp::result<type> result, const intptr_t id) {
		assert_true(static_cast<bool>(result));
		assert_equal(result.status(), concurrencpp::result_status::exception);

		try {
			result.get();
		}
		catch (costume_exception e) {
			return assert_equal(e.id, id);
		}
		catch (...) {}

		assert_true(false);
	}
}

namespace concurrencpp::tests {
	class test_executor : public ::concurrencpp::executor {

	private:
		std::thread m_execution_thread;
		std::thread m_setting_thread;

	public:
		test_executor() noexcept : executor("test_executor") {}

		~test_executor() noexcept {
			if (m_execution_thread.joinable()) {
				m_execution_thread.detach();
			}

			if (m_setting_thread.joinable()) {
				m_setting_thread.detach();
			}
		}

		int max_concurrency_level() const noexcept override {
			return 0;
		}

		bool shutdown_requested() const noexcept override {
			return false;
		}

		void shutdown() noexcept override {
			//do nothing
		}

		bool scheduled_async() const noexcept {
			return std::this_thread::get_id() == m_execution_thread.get_id();
		}

		bool scheduled_inline() const noexcept {
			return std::this_thread::get_id() == m_setting_thread.get_id();
		}

		void enqueue(std::experimental::coroutine_handle<> task) override {
			assert(!m_execution_thread.joinable());
			m_execution_thread = std::thread(task);
		}

		void enqueue(std::span<std::experimental::coroutine_handle<>> span) override {
			(void)span;
			std::abort(); //not neeeded.
		}

		template<class type>
		void set_rp_value(result_promise<type> rp) {
			assert(!m_setting_thread.joinable());
			m_setting_thread = std::thread([rp = std::move(rp)]() mutable {
				std::this_thread::sleep_for(std::chrono::milliseconds(500));
				rp.set_from_function(result_factory<type>::get);
			});
		}

		template<class type>
		size_t set_rp_err(result_promise<type> rp) {
			random randomizer;
			const auto id = static_cast<size_t>(randomizer());

			assert(!m_setting_thread.joinable());
			m_setting_thread = std::thread([id, rp = std::move(rp)]() mutable {
				std::this_thread::sleep_for(std::chrono::milliseconds(500));
				rp.set_exception(std::make_exception_ptr(costume_exception(id)));
			});
			return id;
		}
	};

	inline std::shared_ptr<test_executor> make_test_executor() {
		return std::make_shared<test_executor>();
	}
}

namespace concurrencpp::tests {
	struct throwing_executor : public concurrencpp::executor {

		throwing_executor() : executor("throwing_executor") {}

		void enqueue(std::experimental::coroutine_handle<>) override {
			throw std::runtime_error("executor exception");
		}

		void enqueue(std::span<std::experimental::coroutine_handle<>>) override {
			throw std::runtime_error("executor exception");
		}

		int max_concurrency_level() const noexcept override {
			return 0;
		}

		bool shutdown_requested() const noexcept override {
			return false;
		}

		void shutdown() noexcept override {
			//do nothing
		}
	};

	template<class type>
	void test_executor_error_thrown(
		concurrencpp::result<type> result,
		std::shared_ptr<concurrencpp::executor> throwing_executor) {
		assert_equal(result.status(), concurrencpp::result_status::exception);
		try {
			result.get();
		}
		catch (concurrencpp::errors::executor_exception ex) {
			assert_equal(ex.throwing_executor.get(), throwing_executor.get());

			try {
				std::rethrow_exception(ex.thrown_exception);
			}
			catch (std::runtime_error re) {
				assert_equal(std::string(re.what()), "executor exception");
			}
			catch (...) {
				assert_false(true);
			}
		}
		catch (...) {
			assert_true(false);
		}
	}
}

namespace concurrencpp::tests {
	template<class type>
	class await_to_resolve_coro {

	private:
		result<type> m_result;
		std::shared_ptr<concurrencpp::executor> m_test_executor;
		bool m_force_rescheduling;

	public:
		await_to_resolve_coro(
			result<type> result,
			std::shared_ptr<concurrencpp::executor> test_executor,
			bool force_rescheduling) noexcept :
			m_result(std::move(result)),
			m_test_executor(std::move(test_executor)),
			m_force_rescheduling(force_rescheduling) {}

		result<type> operator() () {
			co_return co_await m_result.await_via(std::move(m_test_executor), m_force_rescheduling);
		}
	};
}

#endif //CONCURRENCPP_RESULT_HELPERS_H
