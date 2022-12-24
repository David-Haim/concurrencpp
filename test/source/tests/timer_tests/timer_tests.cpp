#include "concurrencpp/concurrencpp.h"

#include "infra/tester.h"
#include "infra/assertions.h"
#include "utils/random.h"
#include "utils/object_observer.h"
#include "utils/test_generators.h"
#include "utils/throwing_executor.h"

#include <chrono>

using namespace std::chrono_literals;

namespace concurrencpp::tests {
    void test_one_timer();
    void test_many_timers();
    void test_timer_constructor();

    void test_timer_destructor_empty_timer();
    void test_timer_destructor_dead_timer_queue();
    void test_timer_destructor_functionality();
    void test_timer_destructor_RAII();
    void test_timer_destructor();

    void test_timer_cancel_empty_timer();
    void test_timer_cancel_dead_timer_queue();
    void test_timer_cancel_before_due_time();
    void test_timer_cancel_after_due_time_before_beat();
    void test_timer_cancel_after_due_time_after_beat();
    void test_timer_cancel_RAII();
    void test_timer_cancel();

    void test_timer_operator_bool();

    void test_timer_set_frequency_before_due_time();
    void test_timer_set_frequency_after_due_time();
    void test_timer_set_frequency();

    void test_timer_oneshot_timer();
    void test_timer_delay_object();

    void test_timer_assignment_operator_empty_to_empty();
    void test_timer_assignment_operator_non_empty_to_non_empty();
    void test_timer_assignment_operator_empty_to_non_empty();
    void test_timer_assignment_operator_non_empty_to_empty();
    void test_timer_assignment_operator_assign_to_self();
    void test_timer_assignment_operator();
}  // namespace concurrencpp::tests

using namespace std::chrono;
using concurrencpp::timer;
using time_point = std::chrono::time_point<high_resolution_clock>;

namespace concurrencpp::tests {
    auto empty_callback = []() noexcept {
    };

    class counting_executor : public concurrencpp::executor {

       private:
        std::atomic_size_t m_invocation_count;

       public:
        counting_executor() : executor("counting executor"), m_invocation_count(0) {}

        void enqueue(concurrencpp::task task) override {
            ++m_invocation_count;
            task();
        }

        void enqueue(std::span<concurrencpp::task>) override {
            // do nothing
        }

        int max_concurrency_level() const noexcept override {
            return 0;
        }

        bool shutdown_requested() const noexcept override {
            return false;
        }

        void shutdown() noexcept override {
            // do nothing
        }

        size_t invocation_count() const noexcept {
            return m_invocation_count.load();
        }
    };

    class recording_executor : public concurrencpp::executor {

       public:
        mutable std::mutex m_lock;
        std::vector<::time_point> m_time_points;
        ::time_point m_start_time;

       public:
        recording_executor() : executor("recording executor") {
            m_time_points.reserve(128);
        }

        void enqueue(concurrencpp::task task) override {
            const auto now = high_resolution_clock::now();

            {
                std::unique_lock<std::mutex> lock(m_lock);
                m_time_points.emplace_back(now);
            }

            task();
        }

        void enqueue(std::span<concurrencpp::task>) override {
            // do nothing
        }

        int max_concurrency_level() const noexcept override {
            return 0;
        }

        bool shutdown_requested() const noexcept override {
            return false;
        }

        void shutdown() noexcept override {
            // do nothing
        }

        void start_test() {
            const auto now = high_resolution_clock::now();
            std::unique_lock<std::mutex> lock(m_lock);
            m_start_time = now;
        }

        ::time_point get_start_time() {
            std::unique_lock<std::mutex> lock(m_lock);
            return m_start_time;
        }

        ::time_point get_first_fire_time() {
            std::unique_lock<std::mutex> lock(m_lock);
            assert(!m_time_points.empty());
            return m_time_points[0];
        }

        std::vector<::time_point> flush_time_points() {
            std::unique_lock<std::mutex> lock(m_lock);
            return std::move(m_time_points);
        }

        void reset() {
            std::unique_lock<std::mutex> lock(m_lock);
            m_time_points.clear();
            m_start_time = high_resolution_clock::now();
        }
    };

    class timer_tester {

       private:
        const std::chrono::milliseconds m_due_time;
        std::chrono::milliseconds m_frequency;
        const std::shared_ptr<recording_executor> m_executor = std::make_shared<recording_executor>();
        const std::shared_ptr<timer_queue> m_timer_queue;
        concurrencpp::timer m_timer;

        void assert_timer_stats() noexcept {
            assert_equal(m_timer.get_due_time(), m_due_time);
            assert_equal(m_timer.get_frequency(), m_frequency);
            assert_equal(m_timer.get_executor().get(), m_executor.get());
            assert_equal(m_timer.get_timer_queue().lock(), m_timer_queue);
        }

        size_t calculate_due_time() noexcept {
            const auto start_time = m_executor->get_start_time();
            const auto first_fire = m_executor->get_first_fire_time();
            const auto due_time = duration_cast<milliseconds>(first_fire - start_time).count();
            return static_cast<size_t>(due_time);
        }

        std::vector<size_t> calculate_frequencies() {
            auto fire_times = m_executor->flush_time_points();

            std::vector<size_t> intervals;
            intervals.reserve(fire_times.size());

            for (size_t i = 0; i < fire_times.size() - 1; i++) {
                const auto period = fire_times[i + 1] - fire_times[i];
                const auto interval = duration_cast<milliseconds>(period).count();
                intervals.emplace_back(static_cast<size_t>(interval));
            }

            return intervals;
        }

        void assert_correct_time_points() {
            const auto recorded_due_time = calculate_due_time();
            interval_ok(recorded_due_time, m_due_time.count());

            const auto intervals = calculate_frequencies();
            for (auto tested_frequency : intervals) {
                interval_ok(tested_frequency, m_frequency.count());
            }
        }

       public:
        timer_tester(const milliseconds due_time, const milliseconds frequency) :
            m_due_time(due_time), m_frequency(frequency), m_timer_queue(std::make_shared<timer_queue>(milliseconds(60 * 1000))) {}

        timer_tester(const milliseconds due_time, const milliseconds frequency, const std::shared_ptr<timer_queue>& timer_queue) :
            m_due_time(due_time), m_frequency(frequency), m_timer_queue(timer_queue) {}

        void start_timer_test() {
            m_timer = m_timer_queue->make_timer(m_due_time, m_frequency, m_executor, [] {
            });

            m_executor->start_test();
        }

        void start_once_timer_test() {
            m_timer = m_timer_queue->make_one_shot_timer(m_due_time, m_executor, [] {
            });

            m_executor->start_test();
        }

        void set_new_frequency(std::chrono::milliseconds new_frequency) noexcept {
            m_timer.set_frequency(new_frequency);
            m_executor->reset();
            m_frequency = new_frequency;
        }

        void test_frequencies(size_t expected_frequency) {
            const auto frequencies = calculate_frequencies();
            for (auto tested_frequency : frequencies) {
                interval_ok(tested_frequency, expected_frequency);
            }
        }

        void test() {
            assert_timer_stats();
            assert_correct_time_points();

            m_timer.cancel();
        }

        void test_oneshot_timer() {
            assert_timer_stats();
            interval_ok(calculate_due_time(), m_due_time.count());

            const auto frequencies = calculate_frequencies();
            assert_true(frequencies.empty());

            m_timer.cancel();
        }

        static void interval_ok(size_t interval, size_t expected) noexcept {
            assert_bigger_equal(interval, expected - 30);
            assert_smaller_equal(interval, expected + 100);
        }
    };
}  // namespace concurrencpp::tests

void concurrencpp::tests::test_one_timer() {
    const auto due_time = 1250ms;
    const auto frequency = 250ms;
    const auto tq = std::make_shared<concurrencpp::timer_queue>(milliseconds(60 * 1000));

    timer_tester tester(due_time, frequency, tq);
    tester.start_timer_test();

    std::this_thread::sleep_for(20s);

    tester.test();
}

void concurrencpp::tests::test_many_timers() {
    random randomizer;
    auto timer_queue = std::make_shared<concurrencpp::timer_queue>(milliseconds(60 * 1000));
    std::list<timer_tester> testers;

    const size_t due_time_min = 100;
    const size_t due_time_max = 2'000;
    const size_t frequency_min = 100;
    const size_t frequency_max = 3'000;
    const size_t timer_count = 1'024;

    auto round_down = [](size_t num) {
        return num - num % 50;
    };

    for (size_t i = 0; i < timer_count; i++) {
        const auto due_time = round_down(randomizer(due_time_min, due_time_max));
        const auto frequency = round_down(randomizer(frequency_min, frequency_max));

        testers.emplace_front(milliseconds(due_time), milliseconds(frequency), timer_queue).start_timer_test();
    }

    std::this_thread::sleep_for(5min);

    for (auto& tester : testers) {
        tester.test();
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
    auto executor = std::make_shared<concurrencpp::inline_executor>();

    {
        auto timer_queue = std::make_shared<concurrencpp::timer_queue>(milliseconds(60 * 1000));
        timer = timer_queue->make_timer(10 * 1000ms, 10 * 1000ms, executor, empty_callback);
    }

    // nothing strange should happen here
}

void concurrencpp::tests::test_timer_destructor_functionality() {
    auto executor = std::make_shared<counting_executor>();
    auto timer_queue = std::make_shared<concurrencpp::timer_queue>(120s);

    size_t invocation_count_before = 0;

    {
        auto timer = timer_queue->make_timer(150ms, 150ms, executor, [] {
        });

        std::this_thread::sleep_for(1500ms + 65ms);  // racy test, but supposed to generally work

        invocation_count_before = executor->invocation_count();
    }

    std::this_thread::sleep_for(2s);

    const auto invocation_count_after = executor->invocation_count();
    assert_equal(invocation_count_before, invocation_count_after);
}

void concurrencpp::tests::test_timer_destructor_RAII() {
    const size_t timer_count = 1'024;
    object_observer observer;
    auto timer_queue = std::make_shared<concurrencpp::timer_queue>(120s);
    auto inline_executor = std::make_shared<concurrencpp::inline_executor>();

    std::vector<concurrencpp::timer> timers;
    timers.reserve(timer_count);

    for (size_t i = 0; i < timer_count; i++) {
        timers.emplace_back(timer_queue->make_timer(1s, 1s, inline_executor, observer.get_testing_stub()));
    }

    timers.clear();

    assert_true(observer.wait_destruction_count(timer_count, minutes(2)));
}

void concurrencpp::tests::test_timer_destructor() {
    test_timer_destructor_empty_timer();
    test_timer_destructor_dead_timer_queue();
    test_timer_destructor_functionality();
    test_timer_destructor_RAII();
}

void concurrencpp::tests::test_timer_cancel_empty_timer() {
    timer timer;
    timer.cancel();
}

void concurrencpp::tests::test_timer_cancel_dead_timer_queue() {
    timer timer;

    {
        auto timer_queue = std::make_shared<concurrencpp::timer_queue>(120s);
        auto executor = std::make_shared<concurrencpp::inline_executor>();

        timer = timer_queue->make_timer(10 * 1000ms, 10 * 1000ms, executor, empty_callback);
    }

    timer.cancel();
    assert_false(static_cast<bool>(timer));
}

void concurrencpp::tests::test_timer_cancel_before_due_time() {
    object_observer observer;
    auto timer_queue = std::make_shared<concurrencpp::timer_queue>(120s);
    auto ex = std::make_shared<concurrencpp::inline_executor>();

    auto timer = timer_queue->make_timer(1s, 1s, ex, observer.get_testing_stub());
    timer.cancel();

    assert_false(static_cast<bool>(timer));

    std::this_thread::sleep_for(2s);

    assert_equal(observer.get_execution_count(), static_cast<size_t>(0));
    assert_true(observer.wait_destruction_count(1, 10s));
}

void concurrencpp::tests::test_timer_cancel_after_due_time_before_beat() {
    object_observer observer;
    std::binary_semaphore wc(0);
    auto timer_queue = std::make_shared<concurrencpp::timer_queue>(120s);
    auto ex = std::make_shared<concurrencpp::inline_executor>();

    auto timer = timer_queue->make_timer(100ms, 200ms, ex, [&wc, stub = observer.get_testing_stub()]() mutable {
        stub();
        wc.release();
    });

    // will be released after the first beat.
    wc.acquire();
    timer.cancel();

    std::this_thread::sleep_for(2s);

    assert_false(static_cast<bool>(timer));
    assert_equal(observer.get_execution_count(), static_cast<size_t>(1));
    assert_true(observer.wait_destruction_count(1, 10s));
}

void concurrencpp::tests::test_timer_cancel_after_due_time_after_beat() {
    object_observer observer;
    std::binary_semaphore wc(0);
    auto timer_queue = std::make_shared<concurrencpp::timer_queue>(120s);
    auto ex = std::make_shared<concurrencpp::inline_executor>();
    constexpr size_t max_invocation_count = 4;
    std::atomic_size_t invocation_counter = 0;

    auto timer = timer_queue->make_timer(100ms, 200ms, ex, [&invocation_counter, &wc, stub = observer.get_testing_stub()]() mutable {
        stub();

        const auto c = invocation_counter.fetch_add(1, std::memory_order_relaxed) + 1;
        if (c == max_invocation_count) {
            wc.release();
        }
    });

    // will be released after the first beat.
    wc.acquire();
    timer.cancel();

    std::this_thread::sleep_for(2s);

    assert_false(static_cast<bool>(timer));
    assert_equal(observer.get_execution_count(), static_cast<size_t>(4));
    assert_true(observer.wait_destruction_count(1, 10s));  // one instance was invoked 4 times.
}

void concurrencpp::tests::test_timer_cancel_RAII() {
    const size_t timer_count = 1'024;
    object_observer observer;
    auto timer_queue = std::make_shared<concurrencpp::timer_queue>(120s);
    auto ex = std::make_shared<concurrencpp::inline_executor>();

    std::vector<concurrencpp::timer> timers;
    timers.reserve(timer_count);

    for (size_t i = 0; i < timer_count; i++) {
        timers.emplace_back(timer_queue->make_timer(2s, 2s, ex, observer.get_testing_stub()));
    }

    for (auto& timer : timers) {
        timer.cancel();
    }

    assert_true(observer.wait_destruction_count(timer_count, 1min));
}

void concurrencpp::tests::test_timer_cancel() {
    test_timer_cancel_empty_timer();
    test_timer_cancel_dead_timer_queue();
    test_timer_cancel_before_due_time();
    test_timer_cancel_after_due_time_before_beat();
    test_timer_cancel_after_due_time_after_beat();
    test_timer_cancel_RAII();
}

void concurrencpp::tests::test_timer_operator_bool() {
    auto timer_queue = std::make_shared<concurrencpp::timer_queue>(120s);
    auto inline_executor = std::make_shared<concurrencpp::inline_executor>();

    auto timer_1 = timer_queue->make_timer(10 * 1000ms, 10 * 1000ms, inline_executor, empty_callback);

    assert_true(static_cast<bool>(timer_1));

    auto timer_2 = std::move(timer_1);
    assert_false(static_cast<bool>(timer_1));
    assert_true(static_cast<bool>(timer_2));

    timer_2.cancel();
    assert_false(static_cast<bool>(timer_2));
}

void concurrencpp::tests::test_timer_set_frequency_before_due_time() {
    const auto new_frequency = 100ms;
    timer_tester tester(250ms, 250ms);
    tester.start_timer_test();
    tester.set_new_frequency(new_frequency);

    std::this_thread::sleep_for(std::chrono::seconds(2));

    tester.test_frequencies(new_frequency.count());
}

void concurrencpp::tests::test_timer_set_frequency_after_due_time() {
    const auto new_frequency = 100ms;
    timer_tester tester(150ms, 350ms);
    tester.start_timer_test();

    std::this_thread::sleep_for(215ms);

    tester.set_new_frequency(new_frequency);

    std::this_thread::sleep_for(4s);

    tester.test_frequencies(new_frequency.count());
}

void concurrencpp::tests::test_timer_set_frequency() {
    // empty timer throws
    assert_throws<concurrencpp::errors::empty_timer>([] {
        timer timer;
        timer.set_frequency(200ms);
    });

    test_timer_set_frequency_before_due_time();
    test_timer_set_frequency_after_due_time();
}

void concurrencpp::tests::test_timer_oneshot_timer() {
    timer_tester tester(150ms, 0ms);
    tester.start_once_timer_test();

    std::this_thread::sleep_for(3s);

    tester.test_oneshot_timer();
}

void concurrencpp::tests::test_timer_delay_object() {
    auto timer_queue = std::make_shared<concurrencpp::timer_queue>(120s);
    auto wt_executor = std::make_shared<concurrencpp::worker_thread_executor>();
    const auto expected_interval = 150ms;

    for (size_t i = 0; i < 15; i++) {
        const auto before = high_resolution_clock::now();
        auto delay_object = timer_queue->make_delay_object(expected_interval, wt_executor);
        assert_true(static_cast<bool>(delay_object));
        delay_object.run().get();
        const auto after = high_resolution_clock::now();
        const auto interval_ms = duration_cast<milliseconds>(after - before);
        timer_tester::interval_ok(interval_ms.count(), expected_interval.count());
    }

    wt_executor->shutdown();
}

void concurrencpp::tests::test_timer_assignment_operator_empty_to_empty() {
    concurrencpp::timer timer1, timer2;
    assert_false(static_cast<bool>(timer1));
    assert_false(static_cast<bool>(timer2));

    timer1 = std::move(timer2);

    assert_false(static_cast<bool>(timer1));
    assert_false(static_cast<bool>(timer2));
}

void concurrencpp::tests::test_timer_assignment_operator_non_empty_to_non_empty() {
    auto timer_queue = std::make_shared<concurrencpp::timer_queue>(120s);
    auto executor0 = std::make_shared<inline_executor>();
    auto executor1 = std::make_shared<inline_executor>();

    object_observer observer0, observer1;

    auto timer0 = timer_queue->make_timer(10s, 10s, executor0, observer0.get_testing_stub());
    auto timer1 = timer_queue->make_timer(20s, 20s, executor1, observer1.get_testing_stub());

    timer0 = std::move(timer1);

    assert_true(static_cast<bool>(timer0));
    assert_false(static_cast<bool>(timer1));

    assert_true(observer0.wait_destruction_count(1, 20s));
    assert_equal(observer1.get_destruction_count(), static_cast<size_t>(0));

    assert_equal(timer0.get_due_time(), 20s);
    assert_equal(timer0.get_frequency(), 20s);
    assert_equal(timer0.get_executor(), executor1);
}

void concurrencpp::tests::test_timer_assignment_operator_empty_to_non_empty() {
    auto timer_queue = std::make_shared<concurrencpp::timer_queue>(120s);
    auto executor = std::make_shared<inline_executor>();
    object_observer observer;

    auto timer0 = timer_queue->make_timer(10s, 10s, executor, observer.get_testing_stub());
    concurrencpp::timer timer1;

    timer0 = std::move(timer1);
    assert_false(static_cast<bool>(timer0));
    assert_false(static_cast<bool>(timer1));

    assert_true(observer.wait_destruction_count(1, 20s));
}

void concurrencpp::tests::test_timer_assignment_operator_non_empty_to_empty() {
    auto timer_queue = std::make_shared<concurrencpp::timer_queue>(120s);
    auto executor = std::make_shared<inline_executor>();
    object_observer observer;

    concurrencpp::timer timer0;
    auto timer1 = timer_queue->make_timer(10s, 10s, executor, observer.get_testing_stub());

    timer0 = std::move(timer1);

    assert_false(static_cast<bool>(timer1));
    assert_true(static_cast<bool>(timer0));

    assert_equal(timer0.get_due_time(), 10s);
    assert_equal(timer0.get_frequency(), 10s);
    assert_equal(timer0.get_executor(), executor);
}

void concurrencpp::tests::test_timer_assignment_operator_assign_to_self() {
    auto timer_queue = std::make_shared<concurrencpp::timer_queue>(120s);
    auto executor = std::make_shared<inline_executor>();
    object_observer observer;

    auto timer = timer_queue->make_timer(1s, 1s, executor, observer.get_testing_stub());

    timer = std::move(timer);

    assert_true(static_cast<bool>(timer));
    assert_equal(timer.get_due_time(), 1s);
    assert_equal(timer.get_frequency(), 1s);
    assert_equal(timer.get_executor(), executor);
}

void concurrencpp::tests::test_timer_assignment_operator() {
    test_timer_assignment_operator_empty_to_empty();
    test_timer_assignment_operator_non_empty_to_non_empty();
    test_timer_assignment_operator_empty_to_non_empty();
    test_timer_assignment_operator_non_empty_to_empty();
    test_timer_assignment_operator_assign_to_self();
}

using namespace concurrencpp::tests;

int main() {
    tester test("timer test");

    test.add_step("constructor", test_timer_constructor);
    test.add_step("destructor", test_timer_destructor);
    test.add_step("cancel", test_timer_cancel);
    test.add_step("operator bool", test_timer_operator_bool);
    test.add_step("set_frequency", test_timer_set_frequency);
    test.add_step("oneshot_timer", test_timer_oneshot_timer);
    test.add_step("delay_object", test_timer_delay_object);
    test.add_step("operator =", test_timer_assignment_operator);

    test.launch_test();
    return 0;
}