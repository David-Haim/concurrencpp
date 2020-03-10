#include "concurrencpp.h"
#include "all_tests.h"

#include "../tester/tester.h"
#include "../helpers/assertions.h"
#include "../helpers/object_observer.h"
#include "../helpers/fast_randomizer.h"

#include <chrono>

namespace concurrencpp::tests {
	void test_one_timer();
	void test_many_timers();
	void test_timer_constructor();

	void test_timer_destructor_empty_timer();
	void test_timer_destructor_dead_timer_queue();
	void test_timer_destructor_functionality();
	void test_timer_destructor();

	void test_timer_cancel_empty_timer();
	void test_timer_cancel_dead_timer_queue();
	void test_timer_cancel_before_due_time();
	void test_timer_cancel_after_due_time_before_beat();
	void test_timer_cancel_after_due_time_after_beat();
	void test_timer_cancel();

	void test_timer_operator_bool();

	void test_timer_set_frequency_before_due_time();
	void test_timer_set_frequency_after_due_time();
	void test_timer_set_frequency();

	void test_timer_once();

	::concurrencpp::result<void> test_timer_delay_impl(
		std::shared_ptr<concurrencpp::timer_queue> timer_queue,
		std::shared_ptr<concurrencpp::worker_thread_executor> executor);

	void test_timer_delay();

	void test_timer_assignment_operator_empty_to_empty();
	void test_timer_assignment_operator_non_empty_to_non_empty();
	void test_timer_assignment_operator_empty_to_non_empty();
	void test_timer_assignment_operator_non_empty_to_empty();
	void test_timer_assignment_operator_assign_to_self();
	void test_timer_assignment_operator();
}

using concurrencpp::timer;
using namespace std::chrono;

namespace concurrencpp::tests {
	auto empty_callback = []() noexcept {};

	struct timer_tester_executor :
		public concurrencpp::executor,
		public std::enable_shared_from_this<timer_tester_executor> {

		using time_point = time_point<high_resolution_clock>;

	private:
		mutable std::mutex m_lock;
		std::vector<time_point> m_time_points;
		time_point m_test_start_time;
		object_observer m_observer;
		size_t m_due_time;
		size_t m_frequency;
		std::shared_ptr<concurrencpp::timer_queue> m_timer_queue;

		static void interval_ok(size_t interval, size_t expected) {
			assert_bigger_equal(interval, expected - 50);
			assert_smaller_equal(interval, expected + 200);
		}

		void assert_timer_stats(timer& timer) {
			assert_same(timer.get_due_time(), m_due_time);
			assert_same(timer.get_frequency(), m_frequency);
			assert_same(timer.get_executor().get(), this);
			assert_same(timer.get_timer_queue().lock(), m_timer_queue);
		}

		size_t calculate_due_time() {
			std::unique_lock<std::mutex> lock(m_lock);
			return duration_cast<milliseconds>(m_time_points[0] - m_test_start_time).count();
		}

		std::vector<size_t> calculate_frequencies() {
			std::unique_lock<std::mutex> lock(m_lock);							
			std::vector<size_t> intervals;
			intervals.reserve(m_time_points.size());

			for (size_t i = 0; i < m_time_points.size() - 1; i++) {
				const auto period = m_time_points[i + 1] - m_time_points[i];
				const auto interval = duration_cast<milliseconds>(period).count();
				intervals.emplace_back(interval);
			}

			return intervals;
		}

		void assert_correct_time_points() {
			const auto tested_due_time = calculate_due_time();
			interval_ok(tested_due_time, m_due_time);

			const auto intervals = calculate_frequencies();
			for (auto tested_frequency : intervals) {
				interval_ok(tested_frequency, m_frequency);
			}
		}

		void assert_invoked_corrently() {
			assert_same(m_observer.get_execution_count(), invocation_count());
		}

	public:
		timer_tester_executor(std::shared_ptr<concurrencpp::timer_queue> timer_queue) :
			m_due_time(0),
			m_frequency(0),
			m_timer_queue(timer_queue) {
			m_time_points.reserve(512);
		}

		virtual std::string_view name() const noexcept {
			return "timer_tester_executor";
		}

		virtual void enqueue(task task) {
			std::unique_lock<std::mutex> lock(m_lock);
			m_time_points.emplace_back(high_resolution_clock::now());

			lock.unlock();
			task();
		}

		concurrencpp::timer start_timer_test(size_t due_time, size_t frequency) {
			auto self = shared_from_this();
			auto timer = m_timer_queue->create_timer(
				due_time,
				frequency,
				std::move(self),
				m_observer.get_testing_stub(milliseconds(0)));

			std::unique_lock<std::mutex> lock(m_lock);
			m_due_time = due_time;
			m_frequency = frequency;
			m_test_start_time = high_resolution_clock::now();

			return timer;
		}

		concurrencpp::timer start_once_timer_test(size_t due_time) {
			auto self = shared_from_this();
			auto timer = m_timer_queue->create_one_shot_timer(
				due_time,
				std::move(self),
				m_observer.get_testing_stub(milliseconds(0)));

			std::unique_lock<std::mutex> lock(m_lock);
			m_due_time = due_time;
			m_frequency = static_cast<size_t>(-1);
			m_test_start_time = high_resolution_clock::now();

			return timer;
		}

		size_t invocation_count() {
			std::unique_lock<std::mutex> lock(m_lock);
			return m_time_points.size();
		}

		void reset(size_t new_frequency) {
			std::unique_lock<std::mutex> lock(m_lock);
			m_time_points.clear();
			m_test_start_time = high_resolution_clock::now();
			m_frequency = new_frequency;
		}

		void test_frequencies(size_t frequency) {
			const auto frequencies = calculate_frequencies();
			for (auto tested_frequency : frequencies) {
				interval_ok(tested_frequency, frequency);
			}
		}

		void test(timer& timer) {
			assert_timer_stats(timer);
			assert_correct_time_points();
			assert_invoked_corrently();
		}

		void test_oneshot_timer(timer& timer) {
			assert_timer_stats(timer);
			assert_invoked_corrently();
			
			interval_ok(calculate_due_time(), m_due_time);
		}
	};
}

void concurrencpp::tests::test_one_timer() {
	const size_t due_time = 1270;
	const size_t frequency = 3230;

	auto runtime = make_runtime();
	auto tester = std::make_shared<timer_tester_executor>(runtime->timer_queue());
	auto timer = tester->start_timer_test(due_time, frequency);

	std::this_thread::sleep_for(seconds(30));

	tester->test(timer);
}

void concurrencpp::tests::test_many_timers() {
	auto runtime = make_runtime();
	auto timer_queue = runtime->timer_queue();
	random randomizer;
	
	const size_t due_time_min = 100;
	const size_t due_time_max = 5'000;
	const size_t frequency_min = 100;
	const size_t frequency_max = 5'000;
	const size_t timer_count = 256;

	auto round_down = [](auto num) {
		return num - num % 20;
	};

	std::vector<std::pair<concurrencpp::timer, std::shared_ptr<timer_tester_executor>>> timers;
	timers.reserve(timer_count);

	for (size_t i = 0; i < timer_count; i++) {
		const auto due_time = round_down(randomizer(due_time_min, due_time_max));
		const auto frequency = round_down(randomizer(frequency_min, frequency_max));

		auto tester = std::make_shared<timer_tester_executor>(timer_queue);
		auto timer = tester->start_timer_test(due_time, frequency);

		timers.emplace_back(std::move(timer), std::move(tester));
	}

	std::this_thread::sleep_for(std::chrono::minutes(10));

	for (auto& pair : timers) {
		auto& timer = pair.first;
		auto tester = pair.second;
	
		tester->test(timer);
		timer.cancel();
	}
}

void concurrencpp::tests::test_timer_constructor() {
	test_one_timer();
	test_many_timers();
}

void concurrencpp::tests::test_timer_destructor_empty_timer() {
	timer timer;
}

void concurrencpp::tests::test_timer_destructor_dead_timer_queue() {
	timer timer;
	
	{
		auto runtime = make_runtime();
		auto timer_queue = runtime->timer_queue();
		timer = timer_queue->create_timer(
			10 * 1000,
			10 * 1000,
			runtime->thread_pool_executor(),
			empty_callback);
	}

	//nothing strange should happen here
}

void concurrencpp::tests::test_timer_destructor_functionality() {
	auto runtime = make_runtime();
	auto timer_queue = runtime->timer_queue();
	auto tester = std::make_shared<timer_tester_executor>(timer_queue);

	size_t invocation_count_before = 0;

	{
		auto timer = tester->start_timer_test(500, 500);
		std::this_thread::sleep_for(milliseconds(4150));
		invocation_count_before = tester->invocation_count();
	}

	std::this_thread::sleep_for(seconds(4));

	const auto invocation_count_after = tester->invocation_count();
	assert_same(invocation_count_before, invocation_count_after);
}

void concurrencpp::tests::test_timer_destructor() {
	test_timer_destructor_empty_timer();
	test_timer_destructor_dead_timer_queue();
	test_timer_destructor_functionality();
}

void concurrencpp::tests::test_timer_cancel_empty_timer() {
	timer timer;
	timer.cancel();
}

void concurrencpp::tests::test_timer_cancel_dead_timer_queue() {
	timer timer;

	{
		auto runtime = make_runtime();
		auto timer_queue = runtime->timer_queue();
		timer = timer_queue->create_timer(
			10 * 1000,
			10 * 1000,
			runtime->thread_pool_executor(),
			empty_callback);
	}

	timer.cancel();
	assert_false(timer);
}

void concurrencpp::tests::test_timer_cancel_before_due_time() {
	object_observer observer;
	auto runtime = make_runtime();
	auto timer_queue = runtime->timer_queue();
	auto timer = timer_queue->create_timer(
		1'000,
		500,
		runtime->thread_pool_executor(),
		observer.get_testing_stub(milliseconds(0)));

	timer.cancel();
	assert_false(timer);

	std::this_thread::sleep_for(seconds(4));

	assert_same(observer.get_execution_count(), 0);
	assert_same(observer.get_destruction_count(), 1);
}

void concurrencpp::tests::test_timer_cancel_after_due_time_before_beat() {
	object_observer observer;
	auto runtime = make_runtime();
	auto timer_queue = runtime->timer_queue();
	auto timer = timer_queue->create_timer(
		500,
		500,
		runtime->thread_pool_executor(),
		observer.get_testing_stub(milliseconds(0)));

	std::this_thread::sleep_for(milliseconds(600));

	timer.cancel();
	assert_false(timer);

	std::this_thread::sleep_for(seconds(3));

	assert_same(observer.get_execution_count(), 1);
	assert_same(observer.get_destruction_count(), 1);
}

void concurrencpp::tests::test_timer_cancel_after_due_time_after_beat() {
	object_observer observer;
	auto runtime = make_runtime();
	auto timer_queue = runtime->timer_queue();
	auto timer = timer_queue->create_timer(
		1'000,
		1'000,
		runtime->thread_pool_executor(),
		observer.get_testing_stub(milliseconds(0)));

	std::this_thread::sleep_for(milliseconds(1'000 * 4 + 220));

	const auto execution_count_0 = observer.get_execution_count();

	timer.cancel();
	assert_false(timer);

	std::this_thread::sleep_for(seconds(2));

	const auto execution_count_1 = observer.get_execution_count();

	assert_same(execution_count_0, execution_count_1);
	assert_same(observer.get_destruction_count(), 1);
}

void concurrencpp::tests::test_timer_cancel() {
	test_timer_cancel_empty_timer();
	test_timer_cancel_dead_timer_queue();
	test_timer_cancel_before_due_time();
	test_timer_cancel_after_due_time_before_beat();
	test_timer_cancel_after_due_time_after_beat();
}

void concurrencpp::tests::test_timer_operator_bool() {
	auto runtime = make_runtime();
	auto timer_queue = runtime->timer_queue();

	auto timer_1 = timer_queue->create_timer(
		10 * 1000,
		10 * 1000,
		runtime->inline_executor(),
		empty_callback);

	assert_true(static_cast<bool>(timer_1));

	auto timer_2 = std::move(timer_1);
	assert_false(static_cast<bool>(timer_1));
	assert_true(static_cast<bool>(timer_2));

	timer_2.cancel();
	assert_false(static_cast<bool>(timer_2));
}

void concurrencpp::tests::test_timer_set_frequency_before_due_time() {
	auto runtime = make_runtime();
	auto timer_queue = runtime->timer_queue();
	auto tester = std::make_shared<timer_tester_executor>(timer_queue);
	auto timer = tester->start_timer_test(1'000, 1'000);

	timer.set_frequency(500);
	tester->reset(500);

	std::this_thread::sleep_for(std::chrono::seconds(5));

	tester->test_frequencies(500);
}

void concurrencpp::tests::test_timer_set_frequency_after_due_time() {
	auto runtime = make_runtime();
	auto timer_queue = runtime->timer_queue();
	auto tester = std::make_shared<timer_tester_executor>(timer_queue);
	auto timer = tester->start_timer_test(1'000,1'000);

	std::this_thread::sleep_for(milliseconds(1200));

	timer.set_frequency(500);
	tester->reset(500);

	std::this_thread::sleep_for(std::chrono::seconds(5));

	tester->test_frequencies(500);
}

void concurrencpp::tests::test_timer_set_frequency() {
	assert_throws<concurrencpp::errors::empty_timer>([] {
		timer timer;
		timer.set_frequency(200);
	});

	test_timer_set_frequency_before_due_time();
	test_timer_set_frequency_after_due_time();
}

void concurrencpp::tests::test_timer_once() {
	auto runtime = make_runtime();
	auto timer_queue = runtime->timer_queue();
	auto tester = std::make_shared<timer_tester_executor>(timer_queue);
	auto timer = tester->start_once_timer_test(350);

	assert_same(timer.get_due_time(), 350);
	assert_same(timer.get_frequency(), static_cast<size_t>(-1));
	assert_same(timer.get_executor(), tester);
	assert_same(timer.get_timer_queue().lock(), timer_queue);	

	std::this_thread::sleep_for(seconds(5));

	tester->test_oneshot_timer(timer);
}

concurrencpp::result<void> concurrencpp::tests::test_timer_delay_impl(
	std::shared_ptr<concurrencpp::timer_queue> timer_queue,
	std::shared_ptr<concurrencpp::worker_thread_executor> executor) {
	auto tester = std::make_shared<timer_tester_executor>(timer_queue);

	for (size_t i = 0; i < 10; i++) {
		tester->enqueue([] {});
		auto result = timer_queue->create_delay_object(150, executor);
		assert_true(static_cast<bool>(result));

		co_await result;
	}

	tester->test_frequencies(150);

	tester.reset();
	executor.reset();
	timer_queue.reset();
}

void concurrencpp::tests::test_timer_delay() {
	auto runtime = make_runtime();
	auto timer_queue = runtime->timer_queue();
	auto worker_thread = runtime->worker_thread_executor();
	test_timer_delay_impl(timer_queue, worker_thread).get();
}

void concurrencpp::tests::test_timer_assignment_operator_empty_to_empty() {
	concurrencpp::timer timer1, timer2;
	assert_false(timer1);
	assert_false(timer2);

	timer1 = std::move(timer2);

	assert_false(timer1);
	assert_false(timer2);
}

void concurrencpp::tests::test_timer_assignment_operator_non_empty_to_non_empty() {
	auto runtime = make_runtime();
	auto timer_queue = runtime->timer_queue();

	auto executor0 = runtime->inline_executor();
	auto executor1 = runtime->inline_executor();

	object_observer observer0, observer1;

	auto timer0 = timer_queue->create_timer(10 * 1'000, 10 * 1'000, executor0, observer0.get_testing_stub());
	auto timer1 = timer_queue->create_timer(20 * 1'000, 20 * 1'000, executor1, observer1.get_testing_stub());

	timer0 = std::move(timer1);

	assert_true(timer0);
	assert_false(timer1);

	assert_same(observer0.get_destruction_count(), 1);	
	assert_same(observer1.get_destruction_count(), 0);

	assert_same(timer0.get_due_time(), 20 * 1'000);
	assert_same(timer0.get_frequency(), 20 * 1'000);
	assert_same(timer0.get_executor(), executor1);
}

void concurrencpp::tests::test_timer_assignment_operator_empty_to_non_empty() {
	auto runtime = make_runtime();
	auto timer_queue = runtime->timer_queue();
	auto executor = runtime->inline_executor();
	object_observer observer;

	auto timer0 = timer_queue->create_timer(10 * 1'000, 10 * 1'000, executor, observer.get_testing_stub());
	concurrencpp::timer timer1;

	timer0 = std::move(timer1);

	assert_false(timer0);
	assert_false(timer1);

	assert_same(observer.get_destruction_count(), 1);
}

void concurrencpp::tests::test_timer_assignment_operator_non_empty_to_empty() {
	auto runtime = make_runtime();
	auto timer_queue = runtime->timer_queue();
	auto executor = runtime->inline_executor();
	object_observer observer;

	concurrencpp::timer timer0;
	auto timer1 = timer_queue->create_timer(10 * 1'000, 10 * 1'000, executor, observer.get_testing_stub());

	timer0 = std::move(timer1);

	assert_true(timer0);
	assert_false(timer1);

	assert_same(timer0.get_due_time(), 10 * 1'000);
	assert_same(timer0.get_frequency(), 10 * 1'000);
	assert_same(timer0.get_executor(), executor);
}

void concurrencpp::tests::test_timer_assignment_operator_assign_to_self() {
	auto runtime = make_runtime();
	auto timer_queue = runtime->timer_queue();
	auto executor = runtime->inline_executor();
	object_observer observer;

	auto timer = timer_queue->create_timer(1'000, 1'000, executor, observer.get_testing_stub());

	timer = std::move(timer);

	assert_true(timer);
	assert_same(timer.get_due_time(), 1'000);
	assert_same(timer.get_frequency(), 1'000);
	assert_same(timer.get_executor(), executor);
}

void concurrencpp::tests::test_timer_assignment_operator() {
	test_timer_assignment_operator_empty_to_empty();
	test_timer_assignment_operator_non_empty_to_non_empty();
	test_timer_assignment_operator_empty_to_non_empty();
	test_timer_assignment_operator_non_empty_to_empty();
	test_timer_assignment_operator_assign_to_self();
}

void concurrencpp::tests::test_timer() {
	tester tester("timer test");

	tester.add_step("constructor", test_timer_constructor);
	tester.add_step("destructor", test_timer_destructor);
	tester.add_step("cancel", test_timer_cancel);
	tester.add_step("operator bool", test_timer_operator_bool);
	tester.add_step("set_frequency", test_timer_set_frequency);
	tester.add_step("once", test_timer_once);
	tester.add_step("delay", test_timer_delay);
	tester.add_step("operator =", test_timer_assignment_operator);

	tester.launch_test();
}
