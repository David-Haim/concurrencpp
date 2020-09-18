#ifndef CONCURRENCPP_ALL_TESTS_H
#define CONCURRENCPP_ALL_TESTS_H

namespace concurrencpp::tests {
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

	void test_co_await();
	void test_parallel_coroutine();

	void test_timer();

	void test_runtime();

	void test_all();
}

#endif //CONCURRENCPP_ALL_TESTS_H
