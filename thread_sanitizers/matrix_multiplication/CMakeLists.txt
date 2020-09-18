cmake_minimum_required(VERSION 3.17.0)
project(matrix_multiplication_test)

set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_CXX_STANDARD 20)

set(CLANG_COROUTINES_COMPILE_FLAGS "-Wall -Wextra -fcoroutines-ts -stdlib=libc++ -fsanitize=thread")

set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${CLANG_COROUTINES_COMPILE_FLAGS}")
set(CMAKE_BUILD_TYPE Release)

include_directories(../../concurrencpp/include)

add_executable(
    matrix_multiplication_test
	main.cpp
	)

find_library(LIBRT rt)                                                                                                                                                                                                                                                                                        
	if(LIBRT)
		target_link_libraries(matrix_multiplication_test ${LIBRT})
	endif()
target_link_libraries(matrix_multiplication_test concurrencpp pthread)	
