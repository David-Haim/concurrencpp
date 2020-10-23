#include "concurrencpp/concurrencpp.h"
#include "tests/all_tests.h"

#include "tests/test_utils/test_ready_result.h"

#include "tester/tester.h"
#include "helpers/assertions.h"
#include "helpers/random.h"
#include "helpers/object_observer.h"

namespace concurrencpp::tests {
    void test_initialy_resumed_null_result_promise();
    void test_initialy_resumed_result_promise();
    void test_initialy_rescheduled_null_result_promise();
    void test_initialy_rescheduled_result_promise();
}  // namespace concurrencpp::tests

using worker_ptr = std::shared_ptr<concurrencpp::worker_thread_executor>;

namespace concurrencpp::tests {
    void init_workers(std::span<worker_ptr> span) {
        for (auto& worker : span) {
            worker = std::make_shared<worker_thread_executor>();
        }
    }

    void shutdown_workers(std::span<worker_ptr> span) {
        for (auto& worker : span) {
            worker->shutdown();
        }
    }
}  // namespace concurrencpp::tests

namespace concurrencpp::tests {
    null_result initialy_resumed_null_result_coro(concurrencpp::details::wait_context& wc,
                                                  worker_ptr w0,
                                                  worker_ptr w1,
                                                  worker_ptr w2,
                                                  worker_ptr w3,
                                                  testing_stub stub) {
        int i = 0;
        std::string s = "";

        co_await w0->submit([] {
        });

        ++i;
        s += "a";

        co_await w1->submit([] {
        });

        ++i;
        s += "a";

        co_await w2->submit([] {
        });

        ++i;
        s += "a";

        co_await w3->submit([] {
        });

        assert_equal(i, 3);
        assert_equal(s, "aaa");

        wc.notify();
    }
}  // namespace concurrencpp::tests

void concurrencpp::tests::test_initialy_resumed_null_result_promise() {
    worker_ptr workers[4];
    init_workers(workers);

    object_observer observer;
    concurrencpp::details::wait_context wc;
    initialy_resumed_null_result_coro(wc, workers[0], workers[1], workers[2], workers[3], observer.get_testing_stub());

    wc.wait();
    assert_true(observer.wait_destruction_count(1, std::chrono::seconds(10)));

    shutdown_workers(workers);
}

namespace concurrencpp::tests {
    result<std::pair<int, std::string>> initialy_resumed_result_coro(worker_ptr w0,
                                                                     worker_ptr w1,
                                                                     worker_ptr w2,
                                                                     worker_ptr w3,
                                                                     testing_stub stub,
                                                                     const bool terminate_by_exception) {
        int i = 0;
        std::string s = "";

        co_await w0->submit([] {
        });

        ++i;
        s += "a";

        co_await w1->submit([] {
        });

        ++i;
        s += "a";

        co_await w2->submit([] {
        });

        ++i;
        s += "a";

        co_await w3->submit([] {
        });

        if (terminate_by_exception) {
            throw costume_exception(1234);
        }

        co_return std::make_pair(i, s);
    }
}  // namespace concurrencpp::tests

void concurrencpp::tests::test_initialy_resumed_result_promise() {
    worker_ptr workers[4];
    init_workers(workers);

    object_observer observer;
    auto [i, s] = initialy_resumed_result_coro(workers[0], workers[1], workers[2], workers[3], observer.get_testing_stub(), false).get();

    assert_equal(i, 3);
    assert_equal(s, "aaa");

    assert_true(observer.wait_destruction_count(1, std::chrono::seconds(10)));

    auto result = initialy_resumed_result_coro(workers[0], workers[1], workers[2], workers[3], observer.get_testing_stub(), true);

    result.wait();

    test_ready_result_costume_exception(std::move(result), 1234);
    assert_true(observer.wait_destruction_count(2, std::chrono::seconds(10)));

    shutdown_workers(workers);
}

namespace concurrencpp::tests {
    null_result initialy_rescheduled_null_result_coro(executor_tag,
                                                      worker_ptr w0,
                                                      const std::thread::id caller_thread_id,
                                                      concurrencpp::details::wait_context& wc,
                                                      worker_ptr w1,
                                                      worker_ptr w2,
                                                      worker_ptr w3,
                                                      testing_stub stub) {

        assert_not_equal(caller_thread_id, std::this_thread::get_id());

        int i = 0;
        std::string s = "";

        ++i;
        s += "a";

        co_await w1->submit([] {
        });

        ++i;
        s += "a";

        co_await w2->submit([] {
        });

        ++i;
        s += "a";

        co_await w3->submit([] {
        });

        assert_equal(i, 3);
        assert_equal(s, "aaa");

        wc.notify();
    }
}  // namespace concurrencpp::tests
void concurrencpp::tests::test_initialy_rescheduled_null_result_promise() {
    worker_ptr workers[4];
    init_workers(workers);

    object_observer observer;
    concurrencpp::details::wait_context wc;
    initialy_rescheduled_null_result_coro({}, workers[0], std::this_thread::get_id(), wc, workers[1], workers[2], workers[3], observer.get_testing_stub());

    wc.wait();
    assert_true(observer.wait_destruction_count(1, std::chrono::seconds(10)));

    shutdown_workers(workers);
}

namespace concurrencpp::tests {
    result<std::pair<int, std::string>> initialy_rescheduled_result_coro(executor_tag,
                                                                         worker_ptr w0,
                                                                         const std::thread::id caller_id,
                                                                         worker_ptr w1,
                                                                         worker_ptr w2,
                                                                         worker_ptr w3,
                                                                         testing_stub stub,
                                                                         const bool terminate_by_exception) {

        assert_not_equal(caller_id, std::this_thread::get_id());

        int i = 0;
        std::string s = "";

        co_await w0->submit([] {
        });

        ++i;
        s += "a";

        co_await w1->submit([] {
        });

        ++i;
        s += "a";

        co_await w2->submit([] {
        });

        ++i;
        s += "a";

        co_await w3->submit([] {
        });

        if (terminate_by_exception) {
            throw costume_exception(1234);
        }

        co_return std::make_pair(i, s);
    }
}  // namespace concurrencpp::tests

void concurrencpp::tests::test_initialy_rescheduled_result_promise() {
    worker_ptr workers[4];
    init_workers(workers);

    object_observer observer;
    auto [i, s] =
        initialy_rescheduled_result_coro({}, workers[0], std::this_thread::get_id(), workers[1], workers[2], workers[3], observer.get_testing_stub(), false).get();

    assert_true(observer.wait_destruction_count(1, std::chrono::seconds(10)));

    assert_equal(i, 3);
    assert_equal(s, "aaa");

    auto result =
        initialy_rescheduled_result_coro({}, workers[0], std::this_thread::get_id(), workers[1], workers[2], workers[3], observer.get_testing_stub(), true);

    result.wait();
    test_ready_result_costume_exception(std::move(result), 1234);

    assert_true(observer.wait_destruction_count(2, std::chrono::seconds(10)));

    shutdown_workers(workers);
}

void concurrencpp::tests::test_coroutine_promises() {
    tester tester("coroutine promises test");

    tester.add_step("initialy_resumed_null_result_promise", test_initialy_resumed_null_result_promise);
    tester.add_step("initialy_resumed_result_promise", test_initialy_resumed_result_promise);
    tester.add_step("initialy_rescheduled_null_result_promise", test_initialy_rescheduled_null_result_promise);
    tester.add_step("initialy_rescheduled_result_promise", test_initialy_rescheduled_result_promise);

    tester.launch_test();
}
