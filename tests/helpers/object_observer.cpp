#include "object_observer.h"

namespace concurrencpp::tests::details {
	class object_observer_state {

	private:
		mutable std::mutex m_lock;
		mutable std::condition_variable m_condition;
		std::unordered_map<std::thread::id, size_t> m_execution_map;
		size_t m_destruction_count;
		size_t m_execution_count;
		size_t m_cancellation_count;

	public:
		object_observer_state() :
			m_destruction_count(0),
			m_execution_count(0),
			m_cancellation_count(0) {}

		size_t get_destruction_count() const noexcept {
			std::unique_lock<decltype(m_lock)> lock(m_lock);
			return m_destruction_count;
		}

		size_t get_execution_count() const noexcept {
			std::unique_lock<decltype(m_lock)> lock(m_lock);
			return m_execution_count;
		}

		size_t get_cancellation_count() const noexcept {
			std::unique_lock<decltype(m_lock)> lock(m_lock);
			return m_cancellation_count;
		}

		std::unordered_map<std::thread::id, size_t> get_execution_map() const noexcept {
			std::unique_lock<decltype(m_lock)> lock(m_lock);
			return m_execution_map;
		}

		bool wait_execution_count(size_t count, std::chrono::seconds timeout) {
			std::unique_lock<decltype(m_lock)> lock(m_lock);
			return m_condition.wait_for(lock, timeout, [count, this] {
				return count == m_execution_count;
			});
		}

		void on_execute() {
			const auto this_id = std::this_thread::get_id();

			{
				std::unique_lock<decltype(m_lock)> lock(m_lock);
				++m_execution_count;
				++m_execution_map[this_id];
			}

			m_condition.notify_all();
		}

		bool wait_destruction_count(size_t count, std::chrono::seconds timeout) {
			std::unique_lock<decltype(m_lock)> lock(m_lock);
			return m_condition.wait_for(lock, timeout, [count, this] {
				return count == m_destruction_count;
			});
		}

		void on_destroy() {
			{
				std::unique_lock<decltype(m_lock)> lock(m_lock);
				++m_destruction_count;
			}

			m_condition.notify_all();
		}

		bool wait_cancel_count(size_t count, std::chrono::seconds timeout) {
			std::unique_lock<decltype(m_lock)> lock(m_lock);
			return m_condition.wait_for(lock, timeout, [count, this] {
				return count == m_cancellation_count;
			});
		}

		void on_cancel() {
			{
				std::unique_lock<decltype(m_lock)> lock(m_lock);
				++m_cancellation_count;
			}

			m_condition.notify_all();
		}
	};
}

using concurrencpp::tests::object_observer;

object_observer::object_observer() : m_state(std::make_shared<details::object_observer_state>()) {}

object_observer::testing_stub object_observer::get_testing_stub(std::chrono::milliseconds dummy_work_time) noexcept {
	return { m_state, dummy_work_time };
}

bool object_observer::wait_execution_count(size_t count, std::chrono::seconds timeout) {
	return m_state->wait_execution_count(count, timeout);
}

bool object_observer::wait_destruction_count(size_t count, std::chrono::seconds timeout) {
	return m_state->wait_destruction_count(count, timeout);
}

bool concurrencpp::tests::object_observer::wait_cancel_count(size_t count, std::chrono::seconds timeout){
	return m_state->wait_cancel_count(count, timeout);
}

size_t object_observer::get_destruction_count() const noexcept {
	return m_state->get_destruction_count();
}

size_t object_observer::get_execution_count() const noexcept {
	return m_state->get_execution_count();
}

size_t object_observer::get_cancellation_count() const noexcept {
	return m_state->get_cancellation_count();
}

std::unordered_map<std::thread::id, size_t> object_observer::get_execution_map() const noexcept {
	return m_state->get_execution_map();
}

object_observer::testing_stub::~testing_stub() noexcept {
	if (static_cast<bool>(m_state)) {
		m_state->on_destroy();
	}
}

object_observer::testing_stub& object_observer::testing_stub::operator = (object_observer::testing_stub&& rhs) noexcept {
	if (this == &rhs) {
		return *this;
	}

	if (static_cast<bool>(m_state)) {
		m_state->on_destroy();
	}

	m_state = std::move(rhs.m_state);
	return *this;
}

void object_observer::testing_stub::operator()() noexcept {
	if (static_cast<bool>(m_state)) {
		if (m_dummy_work_time != std::chrono::milliseconds(0)) {
			std::this_thread::sleep_for(m_dummy_work_time);
		}

		m_state->on_execute();
	}
}

void object_observer::testing_stub::cancel(std::exception_ptr error) noexcept {
	if (static_cast<bool>(m_state)) {
		m_state->on_cancel();
	}
}
