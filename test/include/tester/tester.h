#ifndef CONCURRENCPP_TESTER_H
#define CONCURRENCPP_TESTER_H

#include <deque>
#include <functional>

namespace concurrencpp::tests {
    class test_step {

       private:
        const char* m_step_name;
        std::function<void()> m_step;

       public:
        test_step(const char* step_name, std::function<void()> callable);

        void launch_test_step() noexcept;
    };

    class tester {

       private:
        const char* m_test_name;
        std::deque<test_step> m_steps;

       public:
        tester(const char* test_name) noexcept;

        void launch_test() noexcept;
        void add_step(const char* step_name, std::function<void()> callable);
    };

}  // namespace concurrencpp::tests

#endif  // CONCURRENCPP_TESTER_H
