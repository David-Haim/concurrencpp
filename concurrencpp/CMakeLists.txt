cmake_minimum_required(VERSION 3.16)
project(concurrencpp)

add_library(
	concurrencpp
	
	src/runtime/runtime.cpp
	src/threads/thread.cpp
	src/results/promises.cpp
	src/results/result_core.cpp
	src/executors/executor.cpp
	src/executors/executor.cpp
	src/executors/manual_executor.cpp
	src/executors/thread_executor.cpp
	src/executors/thread_pool_executor.cpp
	src/executors/worker_thread_executor.cpp
	src/timers/timer_queue.cpp
	src/timers/timer.cpp
	)

target_include_directories(concurrencpp PUBLIC include)

if (MSVC)
	target_compile_options(concurrencpp PUBLIC /await /permissive-)
else()
	target_compile_options(concurrencpp PUBLIC -stdlib=libc++ -fcoroutines-ts)
	target_link_options(concurrencpp PUBLIC -stdlib=libc++)
	target_link_libraries(concurrencpp PUBLIC pthread)
endif()

find_library(LIBRT rt)
if(LIBRT)
	target_link_libraries(concurrencpp PUBLIC ${LIBRT})
endif()
