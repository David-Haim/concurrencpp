#include "all_tests.h"

void concurrencpp::tests::test_all() {
	test_array_deque();
	test_spin_lock();
	test_task();
	
	test_thread_group();
	test_thread_pool();

	test_result();
	test_result_resolve_all();
	test_result_await_all();	
	test_result_promise();
	
	test_inline_executor();
	test_thread_pool_executor();
	test_background_executor();
	test_thread_executor();
	test_worker_thread_executor();
	test_manual_executor();

	test_co_await();

	test_timer();
}
