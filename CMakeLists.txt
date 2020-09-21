cmake_minimum_required(VERSION 3.16)
project(concurrencpp)

set(CMAKE_CXX_STANDARD 20)

if (MSVC)
	add_compile_options(/W3)
else()
	add_compile_options(-Wall -Wextra)

	option(CONCURRENCPP_ENABLE_TSAN "Enables thread-sanitizer checks" OFF)
	if (CONCURRENCPP_ENABLE_TSAN)
		add_compile_options(-fsanitize=thread)
		add_link_options(-fsanitize=thread)
	endif()
endif()

add_subdirectory(concurrencpp)
add_subdirectory(tests)
add_subdirectory(sandbox)
add_subdirectory(examples)

if (CONCURRENCPP_ENABLE_TSAN)
	add_subdirectory(thread_sanitizers)
endif()
