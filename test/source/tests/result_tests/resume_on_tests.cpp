#include "concurrencpp/concurrencpp.h"

#include "infra/tester.h"
#include "infra/assertions.h"
#include "utils/object_observer.h"
#include "utils/test_generators.h"

#include <unordered_set>

namespace concurrencpp::tests {
    void test_resume_on_null_executor();
    void test_resume_on_shutdown_executor();
    void test_resume_on_shutdown_executor_delayed();
    void test_resume_on_shared_ptr();
    void test_resume_on_ref();
}  // namespace concurrencpp::tests

namespace concurrencpp::tests {
    result<void> resume_on_1_executor(std::shared_ptr<concurrencpp::executor> executor) {
        co_await concurrencpp::resume_on(executor);
    }

    result<void> resume_on_many_executors_shared(std::span<std::shared_ptr<concurrencpp::executor>> executors,
                                                 std::unordered_set<size_t>& set) {
        for (auto& executor : executors) {
            co_await concurrencpp::resume_on(executor);
            set.insert(::concurrencpp::details::thread::get_current_virtual_id());
        }
    }

    result<void> resume_on_many_executors_ref(std::span<concurrencpp::executor*> executors, std::unordered_set<size_t>& set) {
        for (auto executor : executors) {
            co_await concurrencpp::resume_on(*executor);
            set.insert(::concurrencpp::details::thread::get_current_virtual_id());
        }
    }
}  // namespace concurrencpp::tests

void concurrencpp::tests::test_resume_on_null_executor() {
    assert_throws_with_error_message<std::invalid_argument>(
        [] {
            resume_on_1_executor({}).get();
        },
        concurrencpp::details::consts::k_resume_on_null_exception_err_msg);
}

void concurrencpp::tests::test_resume_on_shutdown_executor() {
    auto ex = std::make_shared<inline_executor>();
    ex->shutdown();

    assert_throws<errors::broken_task>([ex] {
        resume_on_1_executor(ex).get();
    });
}

void concurrencpp::tests::test_resume_on_shutdown_executor_delayed() {
    auto ex = std::make_shared<manual_executor>();
    auto result = resume_on_1_executor(ex);

    assert_equal(ex->size(), 1);

    ex->shutdown();

    assert_throws<errors::broken_task>([&result] {
        result.get();
    });
}

void concurrencpp::tests::test_resume_on_shared_ptr() {

    concurrencpp::runtime runtime;
    std::shared_ptr<concurrencpp::executor> executors[4];

    executors[0] = runtime.thread_executor();
    executors[1] = runtime.thread_pool_executor();
    executors[2] = runtime.make_worker_thread_executor();
    executors[3] = runtime.make_worker_thread_executor();

    std::unordered_set<size_t> set;
    resume_on_many_executors_shared(executors, set).get();

    assert_equal(set.size(), std::size(executors));
}

void concurrencpp::tests::test_resume_on_ref() {
    concurrencpp::runtime runtime;
    concurrencpp::executor* executors[4];

    executors[0] = runtime.thread_executor().get();
    executors[1] = runtime.thread_pool_executor().get();
    executors[2] = runtime.make_worker_thread_executor().get();
    executors[3] = runtime.make_worker_thread_executor().get();

    std::unordered_set<size_t> set;
    resume_on_many_executors_ref(executors, set).get();

    assert_equal(set.size(), std::size(executors));
}

using namespace concurrencpp::tests;

int main() {
    tester tester("resume_on");

    tester.add_step("resume_on(nullptr)", test_resume_on_null_executor);
    tester.add_step("resume_on - executor was shut down", test_resume_on_shutdown_executor);
    tester.add_step("resume_on - executor is shut down after enqueuing", test_resume_on_shutdown_executor_delayed);
    tester.add_step("resume_on(std::shared_ptr)", test_resume_on_shared_ptr);
    tester.add_step("resume_on(&)", test_resume_on_ref);

    tester.launch_test();
    return 0;
}