#ifndef CONCURRENCPP_TEST_THREAD_CALLBACKS_H
#define CONCURRENCPP_TEST_THREAD_CALLBACKS_H

#include "infra/assertions.h"
#include "utils/executor_shutdowner.h"
#include "concurrencpp/executors/executor.h"

namespace concurrencpp::tests {
    template<class executor_factory_type>
    void test_thread_callbacks(executor_factory_type executor_factory, std::string_view expected_thread_name) {
        std::atomic_size_t thread_started_callback_invocations_num = 0;
        std::atomic_size_t thread_terminated_callback_invocations_num = 0;

        auto thread_started_callback = [&thread_started_callback_invocations_num, expected_thread_name](std::string_view thread_name) {
            ++thread_started_callback_invocations_num;
            assert_equal(thread_name, expected_thread_name);
        };

        auto thread_terminated_callback = [&thread_terminated_callback_invocations_num,
                                           expected_thread_name](std::string_view thread_name) {
            ++thread_terminated_callback_invocations_num;
            assert_equal(thread_name, expected_thread_name);
        };

        std::shared_ptr<executor> executor = executor_factory(thread_started_callback, thread_terminated_callback);
        executor_shutdowner es(executor);

        assert_equal(thread_started_callback_invocations_num, 0);
        assert_equal(thread_terminated_callback_invocations_num, 0);

        executor
            ->submit([&thread_started_callback_invocations_num, &thread_terminated_callback_invocations_num]() {
                assert_equal(thread_started_callback_invocations_num, 1);
                assert_equal(thread_terminated_callback_invocations_num, 0);
            })
            .get();

        executor->shutdown();
        assert_equal(thread_started_callback_invocations_num, 1);
        assert_equal(thread_terminated_callback_invocations_num, 1);
    }
}  // namespace concurrencpp::tests

#endif
