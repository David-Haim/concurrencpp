cmake_minimum_required(VERSION 3.17.0)
project(concurrencpp)

set(CMAKE_CXX_STANDARD 20)

if (MSVC)
	add_compile_options(/W3 /permissive- /await)
else()
	add_compile_options(-Wall -Wextra -stdlib=libc++ -fcoroutines-ts)
endif()

add_subdirectory(concurrencpp)
add_subdirectory(tests)
add_subdirectory(sandbox)
add_subdirectory(examples)
add_subdirectory(thread_sanitizers)
