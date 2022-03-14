#include "concurrencpp/concurrencpp.h"

#include "infra/tester.h"
#include "infra/assertions.h"
#include "utils/object_observer.h"
#include "utils/test_ready_result.h"

namespace concurrencpp::tests {
    void test_initialy_resumed_null_result_promise_value();
    void test_initialy_resumed_null_result_promise_exception();
    void test_initialy_resumed_null_result_promise();

    void test_initialy_resumed_result_promise_value();
    void test_initialy_resumed_result_promise_exception();
    void test_initialy_resumed_result_promise();

    void test_initialy_rescheduled_null_result_promise_value();
    void test_initialy_rescheduled_null_result_promise_exception();
    void test_initialy_rescheduled_null_result_promise();

    void test_initialy_rescheduled_result_promise_value();
    void test_initialy_rescheduled_result_promise_exception();
    void test_initialy_rescheduled_result_promise();

    void test_lazy_result_promise_value();
    void test_lazy_result_promise_exception();
    void test_lazy_result_promise();
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
    null_result initialy_resumed_null_result_coro(worker_ptr w0,
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

        assert_equal(i, 3);
        assert_equal(s, "aaa");

        if (terminate_by_exception) {
            throw custom_exception(1234);
        }
    }
}  // namespace concurrencpp::tests

void concurrencpp::tests::test_initialy_resumed_null_result_promise_value() {
    worker_ptr workers[4];
    init_workers(workers);

    object_observer observer;
    initialy_resumed_null_result_coro(workers[0], workers[1], workers[2], workers[3], observer.get_testing_stub(), false);

    assert_true(observer.wait_destruction_count(1, std::chrono::seconds(10)));

    shutdown_workers(workers);
}

void concurrencpp::tests::test_initialy_resumed_null_result_promise_exception() {
    worker_ptr workers[4];
    init_workers(workers);

    object_observer observer;
    initialy_resumed_null_result_coro(workers[0], workers[1], workers[2], workers[3], observer.get_testing_stub(), true);

    assert_true(observer.wait_destruction_count(1, std::chrono::seconds(10)));

    shutdown_workers(workers);
}

void concurrencpp::tests::test_initialy_resumed_null_result_promise() {
    test_initialy_resumed_null_result_promise_value();
    test_initialy_resumed_null_result_promise_exception();
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
            throw custom_exception(1234);
        }

        co_return std::make_pair(i, s);
    }
}  // namespace concurrencpp::tests

void concurrencpp::tests::test_initialy_resumed_result_promise_value() {
    worker_ptr workers[4];
    init_workers(workers);

    object_observer observer;
    auto [i, s] =
        initialy_resumed_result_coro(workers[0], workers[1], workers[2], workers[3], observer.get_testing_stub(), false).get();

    assert_equal(i, 3);
    assert_equal(s, "aaa");

    assert_true(observer.wait_destruction_count(1, std::chrono::seconds(10)));

    shutdown_workers(workers);
}

void concurrencpp::tests::test_initialy_resumed_result_promise_exception() {
    worker_ptr workers[4];
    init_workers(workers);

    object_observer observer;
    auto result = initialy_resumed_result_coro(workers[0], workers[1], workers[2], workers[3], observer.get_testing_stub(), true);
    result.wait();

    test_ready_result_custom_exception(std::move(result), 1234);
    assert_true(observer.wait_destruction_count(1, std::chrono::seconds(10)));

    shutdown_workers(workers);
}

void concurrencpp::tests::test_initialy_resumed_result_promise() {
    test_initialy_resumed_result_promise_value();
    test_initialy_resumed_result_promise_exception();
}

namespace concurrencpp::tests {
    null_result initialy_rescheduled_null_result_coro(executor_tag,
                                                      worker_ptr w0,
                                                      const std::thread::id caller_thread_id,
                                                      worker_ptr w1,
                                                      worker_ptr w2,
                                                      worker_ptr w3,
                                                      testing_stub stub,
                                                      const bool terminate_by_exception) {

        assert_not_equal(caller_thread_id, std::this_thread::get_id());

        int i = 0;
        std::string s;

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

        if (terminate_by_exception) {
            throw custom_exception(1234);
        }
    }
}  // namespace concurrencpp::tests

void concurrencpp::tests::test_initialy_rescheduled_null_result_promise_value() {
    worker_ptr workers[4];
    init_workers(workers);

    object_observer observer;
    initialy_rescheduled_null_result_coro({},
                                          workers[0],
                                          std::this_thread::get_id(),
                                          workers[1],
                                          workers[2],
                                          workers[3],
                                          observer.get_testing_stub(),
                                          false);

    assert_true(observer.wait_destruction_count(1, std::chrono::seconds(10)));

    shutdown_workers(workers);
}

void concurrencpp::tests::test_initialy_rescheduled_null_result_promise_exception() {
    worker_ptr workers[4];
    init_workers(workers);

    object_observer observer;
    initialy_rescheduled_null_result_coro({},
                                          workers[0],
                                          std::this_thread::get_id(),
                                          workers[1],
                                          workers[2],
                                          workers[3],
                                          observer.get_testing_stub(),
                                          true);

    assert_true(observer.wait_destruction_count(1, std::chrono::seconds(10)));

    shutdown_workers(workers);
}

namespace concurrencpp::tests {
    null_result null_executor_null_result_coro(executor_tag, std::shared_ptr<executor> ex) {
        co_return;
    }
}  // namespace concurrencpp::tests

void concurrencpp::tests::test_initialy_rescheduled_null_result_promise() {
    // null resume executor
    assert_throws_with_error_message<std::invalid_argument>(
        [] {
            null_executor_null_result_coro({}, {});
        },
        concurrencpp::details::consts::k_parallel_coroutine_null_exception_err_msg);

    test_initialy_rescheduled_null_result_promise_value();
    test_initialy_rescheduled_null_result_promise_exception();
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
        std::string s;

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
            throw custom_exception(1234);
        }

        co_return std::make_pair(i, s);
    }
}  // namespace concurrencpp::tests

void concurrencpp::tests::test_initialy_rescheduled_result_promise_value() {
    worker_ptr workers[4];
    init_workers(workers);

    object_observer observer;
    auto [i, s] = initialy_rescheduled_result_coro({},
                                                   workers[0],
                                                   std::this_thread::get_id(),
                                                   workers[1],
                                                   workers[2],
                                                   workers[3],
                                                   observer.get_testing_stub(),
                                                   false)
                      .get();

    assert_true(observer.wait_destruction_count(1, std::chrono::seconds(10)));

    assert_equal(i, 3);
    assert_equal(s, "aaa");

    shutdown_workers(workers);
}

void concurrencpp::tests::test_initialy_rescheduled_result_promise_exception() {
    worker_ptr workers[4];
    init_workers(workers);

    object_observer observer;

    auto result = initialy_rescheduled_result_coro({},
                                                   workers[0],
                                                   std::this_thread::get_id(),
                                                   workers[1],
                                                   workers[2],
                                                   workers[3],
                                                   observer.get_testing_stub(),
                                                   true);

    result.wait();
    test_ready_result_custom_exception(std::move(result), 1234);

    assert_true(observer.wait_destruction_count(1, std::chrono::seconds(10)));

    shutdown_workers(workers);
}

namespace concurrencpp::tests {
    result<void> null_executor_result_coro(executor_tag, std::shared_ptr<executor> ex) {
        co_return;
    }
}  // namespace concurrencpp::tests

void concurrencpp::tests::test_initialy_rescheduled_result_promise() {
    // null resume executor
    assert_throws_with_error_message<std::invalid_argument>(
        [] {
            null_executor_result_coro({}, {});
        },
        concurrencpp::details::consts::k_parallel_coroutine_null_exception_err_msg);

    test_initialy_rescheduled_result_promise_value();
    test_initialy_rescheduled_result_promise_exception();
}

namespace concurrencpp::tests {
    lazy_result<std::pair<int, std::string>> lazy_result_coro(worker_ptr w0,
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
            throw custom_exception(1234);
        }

        co_return std::make_pair(i, s);
    }
}  // namespace concurrencpp::tests

void concurrencpp::tests::test_lazy_result_promise_value() {
    worker_ptr workers[4];
    init_workers(workers);

    object_observer observer;
    auto [i, s] = lazy_result_coro(workers[0], workers[1], workers[2], workers[3], observer.get_testing_stub(), false).run().get();

    assert_true(observer.wait_destruction_count(1, std::chrono::seconds(10)));

    assert_equal(i, 3);
    assert_equal(s, "aaa");

    shutdown_workers(workers);
}

void concurrencpp::tests::test_lazy_result_promise_exception() {
    worker_ptr workers[4];
    init_workers(workers);

    object_observer observer;

    auto result = lazy_result_coro(workers[0], workers[1], workers[2], workers[3], observer.get_testing_stub(), true).run();

    result.wait();
    test_ready_result_custom_exception(std::move(result), 1234);

    assert_true(observer.wait_destruction_count(1, std::chrono::seconds(10)));

    shutdown_workers(workers);
}

void concurrencpp::tests::test_lazy_result_promise() {
    test_lazy_result_promise_value();
    test_lazy_result_promise_exception();
}

using namespace concurrencpp::tests;

int main() {
    tester tester("coroutine promises test");

    tester.add_step("initialy_resumed_null_result_promise", test_initialy_resumed_null_result_promise);
    tester.add_step("initialy_resumed_result_promise", test_initialy_resumed_result_promise);
    tester.add_step("initialy_rescheduled_null_result_promise", test_initialy_rescheduled_null_result_promise);
    tester.add_step("initialy_rescheduled_result_promise", test_initialy_rescheduled_result_promise);
    tester.add_step("lazy_coroutine_promise", test_lazy_result_promise);

    tester.launch_test();

    return 0;
}