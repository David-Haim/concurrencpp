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

    class testing_stub {

       protected:
        std::shared_ptr<details::object_observer_state> m_state;
        const std::chrono::milliseconds m_dummy_work_time;

       public:
        testing_stub() noexcept : m_dummy_work_time(0) {}

        testing_stub(std::shared_ptr<details::object_observer_state> state, const std::chrono::milliseconds dummy_work_time) noexcept :
            m_state(std::move(state)), m_dummy_work_time(dummy_work_time) {}

        testing_stub(testing_stub&& rhs) noexcept = default;

        ~testing_stub() noexcept;

        testing_stub& operator=(testing_stub&& rhs) noexcept;

        void operator()() noexcept;
    };

    class value_testing_stub : public testing_stub {

       private:
        const size_t m_return_value;

       public:
        value_testing_stub(size_t return_value) noexcept : m_return_value(return_value) {}

        value_testing_stub(std::shared_ptr<details::object_observer_state> state, const std::chrono::milliseconds dummy_work_time, int return_value) noexcept :
            testing_stub(std::move(state), dummy_work_time), m_return_value(return_value) {}

        value_testing_stub(value_testing_stub&& rhs) noexcept = default;

        value_testing_stub& operator=(value_testing_stub&& rhs) noexcept;

        size_t operator()() noexcept;
    };

    class object_observer {

       private:
        const std::shared_ptr<details::object_observer_state> m_state;

       public:
        object_observer();
        object_observer(object_observer&&) noexcept = default;

        testing_stub get_testing_stub(std::chrono::milliseconds dummy_work_time = std::chrono::milliseconds(0)) noexcept;
        value_testing_stub get_testing_stub(int value, std::chrono::milliseconds dummy_work_time = std::chrono::milliseconds(0)) noexcept;
        value_testing_stub get_testing_stub(size_t value, std::chrono::milliseconds dummy_work_time = std::chrono::milliseconds(0)) noexcept;

        bool wait_execution_count(size_t count, std::chrono::milliseconds timeout);
        bool wait_destruction_count(size_t count, std::chrono::milliseconds timeout);

        size_t get_destruction_count() const noexcept;
        size_t get_execution_count() const noexcept;

        std::unordered_map<size_t, size_t> get_execution_map() const noexcept;
    };

}  // namespace concurrencpp::tests

#endif  // !CONCURRENCPP_BEHAVIOURAL_TESTERS_H
