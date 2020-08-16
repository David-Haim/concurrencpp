cmake_minimum_required(VERSION 3.0)
project(executor_test)

set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_CXX_STANDARD 20)

set(CLANG_COROUTINES_COMPILE_FLAGS "-Wall -Wextra -fcoroutines-ts -stdlib=libc++ -fsanitize=thread -g")

set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${CLANG_COROUTINES_COMPILE_FLAGS}")
set(CMAKE_BUILD_TYPE Release)

include_directories(../../concurrencpp/include)

add_executable(
    executor_test
	main.cpp
	)
find_library(LIBRT rt)                                                                                                                                                                                                                                                                                        
	if(LIBRT)
		target_link_libraries(executor_test ${LIBRT})
	endif()
target_link_libraries(executor_test concurrencpp pthread)