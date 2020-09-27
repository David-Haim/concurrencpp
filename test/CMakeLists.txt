cmake_minimum_required(VERSION 3.16)
project(tests)

add_executable(
	tests
	main.cpp
	helpers/assertions.cpp
	helpers/object_observer.cpp

	tester/tester.cpp

	tests/all_tests.cpp
	tests/runtime_tests.cpp

	tests/timer_tests/timer_tests.cpp
	tests/timer_tests/timer_queue_tests.cpp

	tests/executor_tests/inline_executor_tests.cpp
	tests/executor_tests/manual_executor_tests.cpp
	tests/executor_tests/thread_executor_tests.cpp
	tests/executor_tests/thread_pool_executor_tests.cpp
	tests/executor_tests/worker_thread_executor_tests.cpp

	tests/result_tests/result_tests.cpp
	tests/result_tests/result_await_tests.cpp
	tests/result_tests/result_resolve_tests.cpp
	tests/result_tests/result_promise_tests.cpp
	tests/result_tests/make_result_tests.cpp
	tests/result_tests/when_all_tests.cpp
	tests/result_tests/when_any_tests.cpp

	tests/coroutine_tests/coroutine_promises_test.cpp
	tests/coroutine_tests/coroutines_tests.cpp
)

target_link_libraries(tests concurrencpp)
