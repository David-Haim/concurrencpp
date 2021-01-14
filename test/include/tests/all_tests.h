#ifndef CONCURRENCPP_ALL_TESTS_H
#define CONCURRENCPP_ALL_TESTS_H

namespace concurrencpp::tests {
    void test_task();

    void test_derivable_executor();
    void test_inline_executor();
    void test_thread_pool_executor();
    void test_thread_executor();
    void test_worker_thread_executor();
    void test_manual_executor();

    void test_result_promise();
    void test_result();
    void test_result_resolve_all();
    void test_result_await_all();
    void test_result_await();
    void test_make_result();
    void test_when_all();
    void test_when_any();

    void test_shared_result();
    void test_shared_result_await_all();
    void test_shared_result_resolve_all();

    void test_coroutine_promises();
    void test_coroutines();

    void test_timer_queue();
    void test_timer();

    void test_runtime();

    void test_all();
}  // namespace concurrencpp::tests

#endif  // CONCURRENCPP_ALL_TESTS_H
