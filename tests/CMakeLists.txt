cmake_minimum_required(VERSION 3.0)
project(tests)

set(CMAKE_CXX_STANDARD 17)
SET(CMAKE_CXX_COMPILER clang++-6.0)

SET(CLANG_COROUTINES_COMPILE_FLAGS "-Wall -Wextra -std=gnu++2a -fcoroutines-ts -stdlib=libc++")

SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${CLANG_COROUTINES_COMPILE_FLAGS}")
#set(CMAKE_BUILD_TYPE Release)
#set(CMAKE_BUILD_TYPE RELWITHDEBINFO)

include_directories(../concurrencpp/include)

add_executable(
	tests
	main.cpp
	helpers/assertions.cpp
	helpers/object_observer.cpp

	tester/tester.cpp
	tests/all_tests.cpp

	tests/timer_tests.cpp

	tests/utils_tests/array_deque_tests.cpp
	tests/utils_tests/spin_lock_tests.cpp
	tests/utils_tests/task_tests.cpp

	tests/executor_tests/background_executor_tests.cpp
	tests/executor_tests/inline_executor_tests.cpp
	tests/executor_tests/manual_executor_tests.cpp
	tests/executor_tests/thread_executor_tests.cpp
	tests/executor_tests/thread_pool_executor_tests.cpp
	tests/executor_tests/worker_thread_executor_tests.cpp

	tests/result_tests/result_tests.cpp
	tests/result_tests/result_await_tests.cpp
	tests/result_tests/result_resolve_tests.cpp
	tests/result_tests/result_promise_tests.cpp
	tests/result_tests/coroutine_adapters_tests.cpp

	tests/thread_tests/thread_group_tests.cpp
	tests/thread_tests/thread_pool_tests.cpp
)
target_link_libraries(tests concurrencpp pthread rt)
