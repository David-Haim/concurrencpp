cmake_minimum_required(VERSION 3.0)
project(process_monitor)

set(CMAKE_CXX_STANDARD 17)
SET(CMAKE_CXX_COMPILER clang++-6.0)

SET(CLANG_COROUTINES_COMPILE_FLAGS "-Wall -Wextra -std=gnu++2a -fcoroutines-ts -stdlib=libc++")

SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${CLANG_COROUTINES_COMPILE_FLAGS}")
#set(CMAKE_BUILD_TYPE Release)
#set(CMAKE_BUILD_TYPE RELWITHDEBINFO)

include_directories(../../concurrencpp/include)
link_directories(../../build/concurrencpp)

add_executable(
    process_monitor
    src/main.cpp
	src/mock_process_monitor.cpp
)
target_link_libraries(process_monitor concurrencpp pthread rt)
