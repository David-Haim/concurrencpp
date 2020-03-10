#ifndef CONCURRENCPP_OBJECT_OBSERVER_H
#define CONCURRENCPP_OBJECT_OBSERVER_H

#include <chrono>
#include <atomic>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>
#include <unordered_map>
#include <condition_variable>

namespace concurrencpp::tests {

	namespace details {
		class object_observer_state;
	}

	class object_observer {

	private:
		const std::shared_ptr<details::object_observer_state> m_state;

	public:
		class testing_stub {

		private:
			std::shared_ptr<details::object_observer_state> m_state;
			const std::chrono::milliseconds m_dummy_work_time;

		public:
			testing_stub() noexcept : m_dummy_work_time(0){}

			testing_stub(
				std::shared_ptr<details::object_observer_state> state,
				const std::chrono::milliseconds dummy_work_time) noexcept : 
				m_state(std::move(state)),
				m_dummy_work_time(dummy_work_time){}
			
			testing_stub(testing_stub&& rhs) noexcept = default;

			~testing_stub() noexcept;

			testing_stub& operator = (testing_stub&& rhs) noexcept;

			void operator()() noexcept;

			void cancel(std::exception_ptr error) noexcept;
		};

	public:
		object_observer();
		object_observer(object_observer&&) noexcept = default;

		testing_stub get_testing_stub(std::chrono::milliseconds dummy_work_time = std::chrono::milliseconds(15)) noexcept;

		bool wait_execution_count(size_t count, std::chrono::seconds timeout);
		bool wait_destruction_count(size_t count, std::chrono::seconds timeout);
		bool wait_cancel_count(size_t count, std::chrono::seconds timeout);

		size_t get_destruction_count() const noexcept;
		size_t get_execution_count() const noexcept;
		size_t get_cancellation_count() const noexcept;

		std::unordered_map<std::thread::id, size_t> get_execution_map() const noexcept;
	};
}

#endif // !CONCURRENCPP_BEHAVIOURAL_TESTERS_H
