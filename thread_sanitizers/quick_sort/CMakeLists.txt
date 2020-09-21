cmake_minimum_required(VERSION 3.17.0)
project(quick_sort_test)

add_executable(
	quick_sort_test
	main.cpp
	)

target_link_libraries(quick_sort_test concurrencpp)