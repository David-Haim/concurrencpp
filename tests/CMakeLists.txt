cmake_minimum_required(VERSION 3.18.0)
project(tests)

set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_CXX_STANDARD 20)

set(CLANG_COROUTINES_COMPILE_FLAGS "-Wall -Wextra -stdlib=libc++ -fcoroutines-ts")

set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${CLANG_COROUTINES_COMPILE_FLAGS}")
set(CMAKE_BUILD_TYPE Release)
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
	tests/runtime_tests.cpp

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
)
find_library(LIBRT rt)                                                                                                                                                                                                                                                                                        
	if(LIBRT)
		target_link_libraries(tests ${LIBRT})
	endif()
target_link_libraries(tests concurrencpp pthread)
