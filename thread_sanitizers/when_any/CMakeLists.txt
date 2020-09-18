cmake_minimum_required(VERSION 3.17.0)
project(when_any_test)

set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_CXX_STANDARD 20)

set(CLANG_COROUTINES_COMPILE_FLAGS "-Wall -Wextra -fcoroutines-ts -stdlib=libc++ -fsanitize=thread -g")

set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${CLANG_COROUTINES_COMPILE_FLAGS}")
#set(CMAKE_BUILD_TYPE Release)
set(CMAKE_BUILD_TYPE RELWITHDEBINFO)

include_directories(../../concurrencpp/include)

add_executable(
    when_any_test
	main.cpp
	)
	
find_library(LIBRT rt)                                                                                                                                                                                                                                                                                        
	if(LIBRT)
		target_link_libraries(when_any_test ${LIBRT})
	endif()
target_link_libraries(when_any_test concurrencpp pthread)