cmake_minimum_required(VERSION 3.17.0)
project(concurrencpp)

set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_CXX_STANDARD 20)

set(CLANG_COROUTINES_COMPILE_FLAGS "-Wall -Wextra -stdlib=libc++ -fcoroutines-ts")

set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${CLANG_COROUTINES_COMPILE_FLAGS}")
set(CMAKE_BUILD_TYPE Release)
#set(CMAKE_BUILD_TYPE RELWITHDEBINFO)

add_library(
    concurrencpp
	src/executors/executor.cpp
	src/runtime/runtime.cpp
	src/threads/thread.cpp
	src/results/result_core.cpp
	src/executors/executor.cpp
	src/executors/manual_executor.cpp
	src/executors/thread_executor.cpp
	src/executors/thread_pool_executor.cpp
	src/executors/worker_thread_executor.cpp
	src/timers/timer_queue.cpp
	src/timers/timer.cpp
	)

find_library(LIBRT rt)
	if(LIBRT)
		target_link_libraries(concurrencpp ${LIBRT})
	endif()
target_link_libraries(concurrencpp pthread)
