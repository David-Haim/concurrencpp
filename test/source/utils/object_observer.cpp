#include "utils/object_observer.h"
#include "concurrencpp/concurrencpp.h"

#include <mutex>
#include <condition_variable>

namespace concurrencpp::tests::details {
    class object_observer_state {

       private:
        mutable std::mutex m_lock;
        mutable std::condition_variable m_condition;
        std::unordered_map<size_t, size_t> m_execution_map;
        size_t m_destruction_count;
        size_t m_execution_count;

       public:
        object_observer_state() : m_destruction_count(0), m_execution_count(0) {}

        size_t get_destruction_count() const noexcept {
            std::unique_lock<decltype(m_lock)> lock(m_lock);
            return m_destruction_count;
        }

        size_t get_execution_count() const noexcept {
            std::unique_lock<decltype(m_lock)> lock(m_lock);
            return m_execution_count;
        }

        std::unordered_map<size_t, size_t> get_execution_map() const noexcept {
            std::unique_lock<decltype(m_lock)> lock(m_lock);
            return m_execution_map;
        }

        bool wait_execution_count(size_t count, std::chrono::milliseconds timeout) {
            std::unique_lock<decltype(m_lock)> lock(m_lock);
            return m_condition.wait_for(lock, timeout, [count, this] {
                return count == m_execution_count;
            });
        }

        void on_execute() {
            const auto this_id = ::concurrencpp::details::thread::get_current_virtual_id();

            {
                std::unique_lock<decltype(m_lock)> lock(m_lock);
                ++m_execution_count;
                ++m_execution_map[this_id];
            }

            m_condition.notify_all();
        }

        bool wait_destruction_count(size_t count, std::chrono::milliseconds timeout) {
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
    };
}  // namespace concurrencpp::tests::details

using concurrencpp::tests::testing_stub;
using concurrencpp::tests::object_observer;
using concurrencpp::tests::value_testing_stub;

testing_stub& testing_stub::operator=(testing_stub&& rhs) noexcept {
    if (this == &rhs) {
        return *this;
    }

    if (static_cast<bool>(m_state)) {
        m_state->on_destroy();
    }

    m_state = std::move(rhs.m_state);
    return *this;
}

void testing_stub::operator()() noexcept {
    if (static_cast<bool>(m_state)) {
        m_state->on_execute();
    }
}

value_testing_stub& value_testing_stub::operator=(value_testing_stub&& rhs) noexcept {
    testing_stub::operator=(std::move(rhs));
    return *this;
}

size_t value_testing_stub::operator()() noexcept {
    testing_stub::operator()();
    return m_expected_return_value;
}

object_observer::object_observer() : m_state(std::make_shared<details::object_observer_state>()) {}

testing_stub object_observer::get_testing_stub() noexcept {
    return {m_state};
}

value_testing_stub object_observer::get_testing_stub(int value) noexcept {
    return {m_state, value};
}

value_testing_stub object_observer::get_testing_stub(size_t value) noexcept {
    return {m_state, value};
}

bool object_observer::wait_execution_count(size_t count, std::chrono::milliseconds timeout) {
    return m_state->wait_execution_count(count, timeout);
}

bool object_observer::wait_destruction_count(size_t count, std::chrono::milliseconds timeout) {
    return m_state->wait_destruction_count(count, timeout);
}

size_t object_observer::get_destruction_count() const noexcept {
    return m_state->get_destruction_count();
}

size_t object_observer::get_execution_count() const noexcept {
    return m_state->get_execution_count();
}

std::unordered_map<size_t, size_t> object_observer::get_execution_map() const noexcept {
    return m_state->get_execution_map();
}

testing_stub::~testing_stub() noexcept {
    if (static_cast<bool>(m_state)) {
        m_state->on_destroy();
    }
}
