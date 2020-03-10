#ifndef CONCURRENCPP_TESTER_H
#define CONCURRENCPP_TESTER_H

#include <deque>
#include <functional>
#include <chrono>

namespace concurrencpp::tests {

	class test_step {

	private:
		const char* m_step_name;
		std::function<void()> m_step;

	public:
		test_step(const char* step_name, std::function<void()> functor);

		void launch_test_step() noexcept;
	};

	class tester {

	private:
		const char* m_test_name;
		std::deque<test_step> m_steps;

		void start_test();
		void end_test();

	public:
		tester(const char* test_name) noexcept;
		~tester();

		void launch_test() noexcept;
		void add_step(const char* step_name, std::function<void()> functor);
	};

}

#endif //CONCURRENCPP_TESTER_H
