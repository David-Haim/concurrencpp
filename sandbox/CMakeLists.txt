cmake_minimum_required(VERSION 3.17.0)
project(sandbox)

set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_CXX_STANDARD 20)

set(CLANG_COROUTINES_COMPILE_FLAGS "-Wall -Wextra -stdlib=libc++ -fcoroutines-ts")

set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${CLANG_COROUTINES_COMPILE_FLAGS}")
set(CMAKE_BUILD_TYPE Release)
#set(CMAKE_BUILD_TYPE RELWITHDEBINFO)

include_directories(../concurrencpp/include)

add_executable(
    sandbox
    main.cpp
)
find_library(LIBRT rt)                                                                                                                                                                                                                                                                                        
	if(LIBRT)
		target_link_libraries(sandbox ${LIBRT})
	endif()
target_link_libraries(sandbox concurrencpp pthread)

