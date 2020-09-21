cmake_minimum_required(VERSION 3.16)
project(concurrencpp)

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

target_include_directories(concurrencpp PUBLIC include)

find_library(LIBRT rt)
if(LIBRT)
	target_link_libraries(concurrencpp ${LIBRT})
endif()

if (NOT MSVC)
	target_link_libraries(concurrencpp PUBLIC pthread)
endif()
