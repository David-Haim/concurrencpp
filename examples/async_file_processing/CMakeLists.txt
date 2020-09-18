cmake_minimum_required(VERSION 3.17.0)
project(async_file_processing)

set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_CXX_STANDARD 20)

set(CLANG_COROUTINES_COMPILE_FLAGS "-Wall -Wextra -fcoroutines-ts -stdlib=libc++")

set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${CLANG_COROUTINES_COMPILE_FLAGS}")
#set(CMAKE_BUILD_TYPE Release)
#set(CMAKE_BUILD_TYPE RELWITHDEBINFO)

include_directories(../../concurrencpp/include)
link_directories(../../build/concurrencpp)

add_executable(
    async_file_processing
    src/main.cpp
)
find_library(LIBRT rt)                                                                                                                                                                                                                                                                                        
	if(LIBRT)
		target_link_libraries(async_file_processing ${LIBRT})
	endif()
target_link_libraries(async_file_processing concurrencpp pthread)
