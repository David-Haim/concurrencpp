cmake_minimum_required(VERSION 3.0)
project(thread_sanitizers)

if (NOT MSVC)
	add_compile_options(-fsanitize=thread -g)

	add_subdirectory(executors)
	add_subdirectory(result)
	add_subdirectory(fibbonacci)
	add_subdirectory(quick_sort)
	add_subdirectory(matrix_multiplication)
	add_subdirectory(when_all)
	add_subdirectory(when_any)
endif()