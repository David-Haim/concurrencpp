#include "concurrencpp/concurrencpp.h"
#include "tests/all_tests.h"

#include "tester/tester.h"
#include "helpers/assertions.h"
#include "helpers/object_observer.h"
#include "helpers/random.h"

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

using concurrencpp::timer;

using namespace std::chrono;
using time_point = std::chrono::time_point<high_resolution_clock>;

namespace concurrencpp::tests {
auto empty_callback = []() noexcept {
};

class counter_executor : public concurrencpp::executor {

   private:
    std::atomic_size_t m_invocation_count;

   public:
    counter_executor() :
        executor("timer_counter_executor"), m_invocation_count(0) {}

    void enqueue(std::experimental::coroutine_handle<> task) override {
        ++m_invocation_count;
        task();
    }

    void enqueue(std::span<std::experimental::coroutine_handle<>>) override {
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
    recording_executor() :
        executor("timer_recording_executor") {
        m_time_points.reserve(64);
    };

    virtual void enqueue(std::experimental::coroutine_handle<> task) override {
        {
            std::unique_lock<std::mutex> lock(m_lock);
            m_time_points.emplace_back(std::chrono::high_resolution_clock::now());
        }

        task();
    }

    virtual void enqueue(
        std::span<std::experimental::coroutine_handle<>>) override {
        // do nothing
    }

    virtual int max_concurrency_level() const noexcept override {
        return 0;
    }

    virtual bool shutdown_requested() const noexcept override {
        return false;
    }

    virtual void shutdown() noexcept override {
        // do nothing
    }

    void start_test() {
        std::unique_lock<std::mutex> lock(m_lock);
        m_start_time = std::chrono::high_resolution_clock::now();
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
        m_start_time = std::chrono::high_resolution_clock::now();
    }
};

class timer_tester {

   private:
    const std::chrono::milliseconds m_due_time;
    std::chrono::milliseconds m_frequency;
    const std::shared_ptr<recording_executor> m_executor;
    const std::shared_ptr<concurrencpp::timer_queue> m_timer_queue;
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
        const auto due_time =
            duration_cast<milliseconds>(first_fire - start_time).count();
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
        const auto tested_due_time = calculate_due_time();
        interval_ok(tested_due_time, m_due_time.count());

        const auto intervals = calculate_frequencies();
        for (auto tested_frequency : intervals) {
            interval_ok(tested_frequency, m_frequency.count());
        }
    }

   public:
    timer_tester(const std::chrono::milliseconds due_time,
                 const std::chrono::milliseconds frequency,
                 std::shared_ptr<concurrencpp::timer_queue> timer_queue) :
        m_due_time(due_time),
        m_frequency(frequency),
        m_executor(std::make_shared<recording_executor>()),
        m_timer_queue(std::move(timer_queue)) {}

    void start_timer_test() {
        m_timer =
            m_timer_queue->make_timer(m_due_time, m_frequency, m_executor, [] {
            });

        m_executor->start_test();
    }

    void start_once_timer_test() {
        m_timer = m_timer_queue->make_one_shot_timer(m_due_time, m_executor, [] {
        });

        m_executor->start_test();
    }

    void reset(std::chrono::milliseconds new_frequency) noexcept {
        m_timer.set_frequency(new_frequency);
        m_executor->reset();
        m_frequency = new_frequency;
    }

    void test_frequencies(size_t frequency) {
        const auto frequencies = calculate_frequencies();
        for (auto tested_frequency : frequencies) {
            interval_ok(tested_frequency, frequency);
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

        auto frequencies = calculate_frequencies();
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
    const auto due_time = 1270ms;
    const auto frequency = 3230ms;
    auto tq = std::make_shared<concurrencpp::timer_queue>();

    timer_tester tester(due_time, frequency, tq);
    tester.start_timer_test();

    std::this_thread::sleep_for(30s);

    tester.test();
}

void concurrencpp::tests::test_many_timers() {
    random randomizer;
    auto timer_queue = std::make_shared<concurrencpp::timer_queue>();
    std::list<timer_tester> testers;

    const size_t due_time_min = 100;
    const size_t due_time_max = 2'000;
    const size_t frequency_min = 100;
    const size_t frequency_max = 3'000;
    const size_t timer_count = 1'024 * 4;

    auto round_down = [](size_t num) {
        return num - num % 50;
    };

    for (size_t i = 0; i < timer_count; i++) {
        const auto due_time = round_down(randomizer(due_time_min, due_time_max));
        const auto frequency = round_down(randomizer(frequency_min, frequency_max));

        testers
            .emplace_front(std::chrono::milliseconds(due_time),
                           std::chrono::milliseconds(frequency),
                           timer_queue)
            .start_timer_test();
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
        auto timer_queue = std::make_shared<concurrencpp::timer_queue>();

        timer = timer_queue->make_timer(10 * 1000ms, 10 * 1000ms, executor, empty_callback);
    }

    // nothing strange should happen here
}

void concurrencpp::tests::test_timer_destructor_functionality() {
    auto executor = std::make_shared<counter_executor>();
    auto timer_queue = std::make_shared<concurrencpp::timer_queue>();

    size_t invocation_count_before = 0;

    {
        auto timer = timer_queue->make_timer(500ms, 500ms, executor, [] {
        });

        std::this_thread::sleep_for(4150ms);

        invocation_count_before = executor->invocation_count();
    }

    std::this_thread::sleep_for(4s);

    const auto invocation_count_after = executor->invocation_count();
    assert_equal(invocation_count_before, invocation_count_after);
}

void concurrencpp::tests::test_timer_destructor_RAII() {
    const size_t timer_count = 1'024;
    object_observer observer;
    auto timer_queue = std::make_shared<concurrencpp::timer_queue>();
    auto inline_executor = std::make_shared<concurrencpp::inline_executor>();

    std::vector<concurrencpp::timer> timers;
    timers.reserve(timer_count);

    for (size_t i = 0; i < timer_count; i++) {
        timers.emplace_back(timer_queue->make_timer(
            1'000ms,
            1'000ms,
            inline_executor,
            observer.get_testing_stub()));
    }

    timers.clear();
    timer_queue.reset();

    assert_equal(observer.get_destruction_count(), timer_count);
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
        auto timer_queue = std::make_shared<concurrencpp::timer_queue>();
        auto executor = std::make_shared<concurrencpp::inline_executor>();

        timer = timer_queue->make_timer(10 * 1000ms, 10 * 1000ms, executor, empty_callback);
    }

    timer.cancel();
    assert_false(timer);
}

void concurrencpp::tests::test_timer_cancel_before_due_time() {
    object_observer observer;
    auto timer_queue = std::make_shared<concurrencpp::timer_queue>();
    auto wt_executor = std::make_shared<concurrencpp::worker_thread_executor>();

    auto timer = timer_queue->make_timer(1'000ms, 500ms, wt_executor, observer.get_testing_stub());

    timer.cancel();
    assert_false(timer);

    std::this_thread::sleep_for(4s);

    assert_equal(observer.get_execution_count(), size_t(0));
    assert_equal(observer.get_destruction_count(), size_t(1));

    wt_executor->shutdown();
}

void concurrencpp::tests::test_timer_cancel_after_due_time_before_beat() {
    object_observer observer;
    auto timer_queue = std::make_shared<concurrencpp::timer_queue>();
    auto wt_executor = std::make_shared<concurrencpp::worker_thread_executor>();

    auto timer = timer_queue->make_timer(500ms, 500ms, wt_executor, observer.get_testing_stub());

    std::this_thread::sleep_for(std::chrono::milliseconds(600));

    timer.cancel();
    assert_false(timer);

    std::this_thread::sleep_for(std::chrono::seconds(3));

    assert_equal(observer.get_execution_count(), size_t(1));
    assert_equal(observer.get_destruction_count(), size_t(1));

    wt_executor->shutdown();
}

void concurrencpp::tests::test_timer_cancel_after_due_time_after_beat() {
    object_observer observer;
    auto timer_queue = std::make_shared<concurrencpp::timer_queue>();
    auto wt_executor = std::make_shared<concurrencpp::worker_thread_executor>();

    auto timer = timer_queue->make_timer(1'000ms, 1'000ms, wt_executor, observer.get_testing_stub());

    std::this_thread::sleep_for(std::chrono::milliseconds(1'000 * 4 + 220));

    const auto execution_count_0 = observer.get_execution_count();

    timer.cancel();
    assert_false(timer);

    std::this_thread::sleep_for(std::chrono::seconds(4));

    const auto execution_count_1 = observer.get_execution_count();

    assert_equal(execution_count_0, execution_count_1);
    assert_equal(observer.get_destruction_count(), size_t(1));

    wt_executor->shutdown();
}

void concurrencpp::tests::test_timer_cancel_RAII() {
    const size_t timer_count = 1'024;
    object_observer observer;
    auto timer_queue = std::make_shared<concurrencpp::timer_queue>();
    auto inline_executor = std::make_shared<concurrencpp::inline_executor>();

    std::vector<concurrencpp::timer> timers;
    timers.reserve(timer_count);

    for (size_t i = 0; i < timer_count; i++) {
        timers.emplace_back(timer_queue->make_timer(
            1'000ms,
            1'000ms,
            inline_executor,
            observer.get_testing_stub()));
    }

    timer_queue.reset();

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
    auto timer_queue = std::make_shared<concurrencpp::timer_queue>();
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
    auto timer_queue = std::make_shared<concurrencpp::timer_queue>();
    timer_tester tester(1'000ms, 1'000ms, timer_queue);
    tester.start_timer_test();

    tester.reset(500ms);

    std::this_thread::sleep_for(std::chrono::seconds(5));

    tester.test_frequencies(500);
}

void concurrencpp::tests::test_timer_set_frequency_after_due_time() {
    auto timer_queue = std::make_shared<concurrencpp::timer_queue>();
    timer_tester tester(1'000ms, 1'000ms, timer_queue);
    tester.start_timer_test();

    std::this_thread::sleep_for(1200ms);

    tester.reset(500ms);

    std::this_thread::sleep_for(5s);

    tester.test_frequencies(500);
}

void concurrencpp::tests::test_timer_set_frequency() {
    assert_throws<concurrencpp::errors::empty_timer>([] {
        timer timer;
        timer.set_frequency(200ms);
    });

    test_timer_set_frequency_before_due_time();
    test_timer_set_frequency_after_due_time();
}

void concurrencpp::tests::test_timer_oneshot_timer() {
    auto timer_queue = std::make_shared<concurrencpp::timer_queue>();
    timer_tester tester(350ms, 0ms, timer_queue);
    tester.start_once_timer_test();

    std::this_thread::sleep_for(5s);

    tester.test_oneshot_timer();
}

void concurrencpp::tests::test_timer_delay_object() {
    auto timer_queue = std::make_shared<concurrencpp::timer_queue>();
    auto wt_executor = std::make_shared<concurrencpp::worker_thread_executor>();
    const auto expected_interval = 150ms;

    for (size_t i = 0; i < 15; i++) {
        const auto before = std::chrono::high_resolution_clock::now();
        auto delay_object =
            timer_queue->make_delay_object(expected_interval, wt_executor);
        assert_true(static_cast<bool>(delay_object));
        delay_object.get();
        const auto after = std::chrono::high_resolution_clock::now();
        const auto interval_ms = duration_cast<milliseconds>(after - before);
        timer_tester::interval_ok(interval_ms.count(), expected_interval.count());
    }

    wt_executor->shutdown();
}

void concurrencpp::tests::test_timer_assignment_operator_empty_to_empty() {
    concurrencpp::timer timer1, timer2;
    assert_false(timer1);
    assert_false(timer2);

    timer1 = std::move(timer2);

    assert_false(timer1);
    assert_false(timer2);
}

void concurrencpp::tests::
    test_timer_assignment_operator_non_empty_to_non_empty() {
    auto timer_queue = std::make_shared<concurrencpp::timer_queue>();

    auto executor0 = std::make_shared<inline_executor>();
    auto executor1 = std::make_shared<inline_executor>();

    object_observer observer0, observer1;

    auto timer0 = timer_queue->make_timer(10 * 1'000ms, 10 * 1'000ms, executor0, observer0.get_testing_stub());
    auto timer1 = timer_queue->make_timer(20 * 1'000ms, 20 * 1'000ms, executor1, observer1.get_testing_stub());

    timer0 = std::move(timer1);

    assert_true(timer0);
    assert_false(timer1);

    assert_true(observer0.wait_destruction_count(1, 20s));
    assert_equal(observer1.get_destruction_count(), size_t(0));

    assert_equal(timer0.get_due_time(), 20'000ms);
    assert_equal(timer0.get_frequency(), 20'000ms);
    assert_equal(timer0.get_executor(), executor1);
}

void concurrencpp::tests::test_timer_assignment_operator_empty_to_non_empty() {
    auto timer_queue = std::make_shared<concurrencpp::timer_queue>();
    auto executor = std::make_shared<inline_executor>();
    object_observer observer;

    auto timer0 = timer_queue->make_timer(10 * 1'000ms, 10 * 1'000ms, executor, observer.get_testing_stub());
    concurrencpp::timer timer1;

    timer0 = std::move(timer1);

    assert_false(timer0);
    assert_false(timer1);

    assert_true(observer.wait_destruction_count(1, 20s));
}

void concurrencpp::tests::test_timer_assignment_operator_non_empty_to_empty() {
    auto timer_queue = std::make_shared<concurrencpp::timer_queue>();
    auto executor = std::make_shared<inline_executor>();
    object_observer observer;

    concurrencpp::timer timer0;
    auto timer1 = timer_queue->make_timer(10 * 1'000ms, 10 * 1'000ms, executor, observer.get_testing_stub());

    timer0 = std::move(timer1);

    assert_true(timer0);
    assert_false(timer1);

    assert_equal(timer0.get_due_time(), 10'000ms);
    assert_equal(timer0.get_frequency(), 10'000ms);
    assert_equal(timer0.get_executor(), executor);
}

void concurrencpp::tests::test_timer_assignment_operator_assign_to_self() {
    auto timer_queue = std::make_shared<concurrencpp::timer_queue>();
    auto executor = std::make_shared<inline_executor>();
    object_observer observer;

    auto timer = timer_queue->make_timer(1'000ms, 1'000ms, executor, observer.get_testing_stub());

    timer = std::move(timer);

    assert_true(timer);
    assert_equal(timer.get_due_time(), 1'000ms);
    assert_equal(timer.get_frequency(), 1'000ms);
    assert_equal(timer.get_executor(), executor);
}

void concurrencpp::tests::test_timer_assignment_operator() {
    test_timer_assignment_operator_empty_to_empty();
    test_timer_assignment_operator_non_empty_to_non_empty();
    test_timer_assignment_operator_empty_to_non_empty();
    test_timer_assignment_operator_non_empty_to_empty();
    test_timer_assignment_operator_assign_to_self();
}

void concurrencpp::tests::test_timer() {
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
}
