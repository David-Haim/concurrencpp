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
	src/executors/task.cpp
	src/executors/executors.cpp
	src/runtime/runtime.cpp
	src/threads/thread_group.cpp
	src/threads/thread_pool.cpp
	src/threads/single_worker_thread.cpp
	src/timers/timer_queue.cpp
	src/timers/timer.cpp
	)
target_link_libraries(concurrencpp pthread rt)
