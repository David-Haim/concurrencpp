#include "concurrencpp/concurrencpp.h"

#include "infra/tester.h"
#include "infra/assertions.h"

namespace concurrencpp::tests {
    void test_runtime_constructor();
    void test_runtime_destructor();
    void test_runtime_version();
}  // namespace concurrencpp::tests

namespace concurrencpp::tests {
    struct dummy_executor : public concurrencpp::executor {
        bool shutdown_requested_flag = false;

        dummy_executor(const char* name, int, float) : executor(name) {}

        void enqueue(concurrencpp::task) override {}
        void enqueue(std::span<concurrencpp::task>) override {}

        int max_concurrency_level() const noexcept override {
            return 0;
        }

        bool shutdown_requested() const noexcept override {
            return shutdown_requested_flag;
        }

        void shutdown() noexcept override {
            shutdown_requested_flag = true;
        }
    };
}  // namespace concurrencpp::tests

void concurrencpp::tests::test_runtime_constructor() {
    concurrencpp::runtime_options opts;
    opts.max_cpu_threads = 3;
    opts.max_thread_pool_executor_waiting_time = std::chrono::milliseconds(12345);

    opts.max_background_threads = 7;
    opts.max_background_executor_waiting_time = std::chrono::milliseconds(54321);

    std::atomic_size_t thread_started_callback_invocations_num = 0;
    std::atomic_size_t thread_terminated_callback_invocations_num = 0;

    opts.thread_started_callback = [&thread_started_callback_invocations_num](std::string_view thread_name) {
        assert_false(thread_name.empty());
        ++thread_started_callback_invocations_num;
    };

    opts.thread_terminated_callback = [&thread_terminated_callback_invocations_num](std::string_view thread_name) {
        assert_false(thread_name.empty());
        ++thread_terminated_callback_invocations_num;
    };

    concurrencpp::runtime runtime(opts);
    auto dummy_ex = runtime.make_executor<dummy_executor>("dummy_executor", 1, 4.4f);
    assert_true(static_cast<bool>(runtime.inline_executor()));
    assert_true(static_cast<bool>(runtime.thread_executor()));
    assert_true(static_cast<bool>(runtime.thread_pool_executor()));
    assert_true(static_cast<bool>(runtime.background_executor()));
    assert_true(static_cast<bool>(dummy_ex));

    assert_false(runtime.inline_executor()->shutdown_requested());
    assert_false(runtime.thread_executor()->shutdown_requested());
    assert_false(runtime.thread_pool_executor()->shutdown_requested());
    assert_false(runtime.background_executor()->shutdown_requested());
    assert_false(dummy_ex->shutdown_requested());

    assert_equal(runtime.thread_pool_executor()->max_concurrency_level(), opts.max_cpu_threads);
    assert_equal(runtime.thread_pool_executor()->max_worker_idle_time(), opts.max_thread_pool_executor_waiting_time);
    assert_equal(runtime.background_executor()->max_concurrency_level(), opts.max_background_threads);
    assert_equal(runtime.background_executor()->max_worker_idle_time(), opts.max_background_executor_waiting_time);

    auto test_runtime_executor = [&thread_started_callback_invocations_num,
                                  &thread_terminated_callback_invocations_num](std::shared_ptr<executor> executor) {
        thread_started_callback_invocations_num = 0;
        thread_terminated_callback_invocations_num = 0;

        executor
            ->submit([&thread_started_callback_invocations_num, &thread_terminated_callback_invocations_num]() {
                assert_equal(thread_started_callback_invocations_num, 1);
                assert_equal(thread_terminated_callback_invocations_num, 0);
            })
            .get();

        executor->shutdown();
        assert_equal(thread_started_callback_invocations_num, 1);
        assert_equal(thread_terminated_callback_invocations_num, 1);
    };

    test_runtime_executor(runtime.thread_executor());
    test_runtime_executor(runtime.thread_pool_executor());
    test_runtime_executor(runtime.background_executor());
}

void concurrencpp::tests::test_runtime_destructor() {
    std::shared_ptr<concurrencpp::executor> executors[7];

    {
        concurrencpp::runtime runtime;
        executors[0] = runtime.inline_executor();
        executors[1] = runtime.thread_pool_executor();
        executors[2] = runtime.background_executor();
        executors[3] = runtime.thread_executor();
        executors[4] = runtime.make_worker_thread_executor();
        executors[5] = runtime.make_manual_executor();
        executors[6] = runtime.make_executor<dummy_executor>("dummy_executor", 1, 4.4f);
    }

    for (auto& executor : executors) {
        assert_true(executor->shutdown_requested());
    }
}

void concurrencpp::tests::test_runtime_version() {
    auto version = concurrencpp::runtime::version();
    assert_equal(std::get<0>(version), concurrencpp::details::consts::k_concurrencpp_version_major);
    assert_equal(std::get<1>(version), concurrencpp::details::consts::k_concurrencpp_version_minor);
    assert_equal(std::get<2>(version), concurrencpp::details::consts::k_concurrencpp_version_revision);
}

using namespace concurrencpp::tests;

int main() {
    tester tester("runtime test");

    tester.add_step("constructor", test_runtime_constructor);
    tester.add_step("destructor", test_runtime_destructor);
    tester.add_step("version", test_runtime_version);

    tester.launch_test();
    return 0;
}