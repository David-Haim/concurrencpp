cmake_minimum_required(VERSION 3.0)
project(concurrencpp)

set(CMAKE_CXX_STANDARD 17)
SET(CMAKE_CXX_COMPILER clang++-6.0)

SET(CLANG_COROUTINES_COMPILE_FLAGS "-Wall -Wextra -std=gnu++2a -fcoroutines-ts -stdlib=libc++")

SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${CLANG_COROUTINES_COMPILE_FLAGS}")
set(CMAKE_BUILD_TYPE Release)
#set(CMAKE_BUILD_TYPE RELWITHDEBINFO)

add_library(
    concurrencpp
	details/executors/task.cpp
	details/executors/executors.cpp
	details/runtime/runtime.cpp
	details/threads/thread_group.cpp
	details/threads/thread_pool.cpp
	details/threads/single_worker_thread.cpp
	details/timers/timer_queue.cpp
	details/timers/timer.cpp
	)
target_link_libraries(concurrencpp pthread rt)
