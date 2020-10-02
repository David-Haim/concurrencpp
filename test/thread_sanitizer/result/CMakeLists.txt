cmake_minimum_required(VERSION 3.16)
project(result_test)

add_executable(
	result_test
	main.cpp
	)

target_link_libraries(result_test concurrencpp)
