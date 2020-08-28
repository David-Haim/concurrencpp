#include "all_tests.h"

void concurrencpp::tests::test_all() {
	test_inline_executor();
	test_thread_pool_executor();
	test_thread_executor();
	test_worker_thread_executor();
	test_manual_executor();

	test_result();
	test_result_resolve_all();
	test_result_await_all();
	test_result_promise();
	test_make_result();

	test_co_await();
	test_parallel_coroutine();

	test_timer();

	test_runtime();
}
